# System Architecture

## Write-Through Caching NVMe Accelerator Simulator

**Project:** Write-Through Caching NVMe Accelerator Simulator  
**Stage:** Stage 3 — System Design & Architecture  
**Language:** C++17  
**Platform:** Linux / Ubuntu under WSL2

---

# 1. Architecture Overview

The Write-Through Caching NVMe Accelerator Simulator is a Linux/C++ software system designed to study how write-through caching strategies interact with storage parallelism.

The system compares two implementations:

1. **Baseline Write-Through Cache**
2. **Optimized Write-Through Cache**

Both implementations use the same software-modeled NVMe device and the same backing storage mechanism. This allows the performance behavior of the two cache strategies to be compared under controlled workloads.

The optimized implementation introduces:

- Write coalescing
- Batch formation
- Background flushing
- Multi-queue dispatch
- Concurrent outstanding writes

The NVMe component is a **software simulation of NVMe-style queue architecture**. It is not a physical NVMe controller.

---

# 2. High-Level Architecture

The overall architecture is:

```text
                         ┌─────────────────────┐
                         │  Workload Generator │
                         └──────────┬──────────┘
                                    │
                                    ▼
                         ┌─────────────────────┐
                         │   Application Layer │
                         │    Write Requests   │
                         └──────────┬──────────┘
                                    │
                         ┌──────────┴──────────┐
                         │                     │
                         ▼                     ▼
                ┌─────────────────┐   ┌─────────────────────┐
                │ Baseline Cache  │   │  Optimized Cache    │
                │                 │   │                     │
                │ Synchronous     │   │ Write Coalescing    │
                │ Write-Through   │   │ Batching            │
                │                 │   │ Background Flush    │
                └────────┬────────┘   └──────────┬──────────┘
                         │                       │
                         │                       ▼
                         │             ┌─────────────────────┐
                         │             │ NVMe-Style Device   │
                         │             │                     │
                         │             │ Queue 0             │
                         │             │ Queue 1             │
                         │             │ Queue 2             │
                         │             │ ...                 │
                         │             │ Queue N             │
                         │             └──────────┬──────────┘
                         │                       │
                         └───────────┬───────────┘
                                     │
                                     ▼
                           ┌─────────────────────┐
                           │   Storage Backend   │
                           │                     │
                           │ O_DIRECT + pwrite() │
                           │     Backing File    │
                           └──────────┬──────────┘
                                      │
                                      ▼
                           ┌─────────────────────┐
                           │  Completion / ACK   │
                           └──────────┬──────────┘
                                      │
                                      ▼
                           ┌─────────────────────┐
                           │  Metrics Collector  │
                           └─────────────────────┘
```

# 3. Major System Components

The system consists of the following major components:

| Component | Main Responsibility |
| --- | --- |
| Workload Generator | Generates controlled write requests |
| Baseline Cache | Provides reference synchronous write-through behavior |
| Optimized Cache | Implements coalescing, batching and multi-queue dispatch |
| Write Coalescer | Combines pending writes targeting the same LBA |
| Batch/Flush Manager | Extracts pending writes and submits batches |
| NVMe Device | Manages software-simulated NVMe queue pairs |
| NVMe Queue Pair | Processes submitted commands using a worker thread |
| Storage Backend | Performs actual file I/O using Linux system calls |
| Metrics Collector | Records latency and performance statistics |
| Benchmark Runner | Controls experiments and compares implementations |
| Result Generator | Produces CSV, logs and visualization data |

# 4. Workload Generator

The workload generator creates the logical write requests used by the benchmark.

It supports configurable parameters such as:

- Number of requests
- Address-space size
- Distribution
- Zipf skew
- Random seed
- Application thread count

Supported workload distributions include:

- Uniform
- Zipf

The workload generator allows different locality patterns to be tested.

For example, a uniform workload distributes requests broadly across the address space, while a Zipf workload creates higher locality around frequently accessed LBAs.

# 5. Application Layer

The application layer represents the source of logical write requests.

Multiple application threads may generate writes concurrently.

Each request contains at least:

```text
LBA
Write Data
Request Timing
```

The same logical workload can be passed through both cache implementations so that their behavior can be compared.

# 6. Baseline Write-Through Cache

The baseline cache provides the reference implementation.

Its write path is intentionally simple:

```text
Application Request
        │
        ▼
Update Cache
        │
        ▼
Create Aligned Buffer
        │
        ▼
Submit to Queue 0
        │
        ▼
Wait for Completion
        │
        ▼
Return to Application
```

The baseline implementation uses:

- One physical write per logical write
- Queue 0
- Synchronous completion waiting
- No write coalescing
- No batching
- No multi-queue pipeline

This establishes the reference behavior against which the optimized implementation is compared.

# 7. Optimized Write-Through Cache

The optimized cache preserves write-through semantics while changing how writes are organized before they reach the storage layer.

## 7.1 Write Path

```text
Application Request
        │
        ▼
Update Cache
        │
        ▼
Add to Pending Write Table
        │
        ▼
Coalesce Same-LBA Writes
        │
        ▼
Batch Trigger / Flush Interval
        │
        ▼
Extract Pending Batch
        │
        ▼
Dispatch Across NVMe-Style Queues
        │
        ▼
Concurrent Physical Writes
        │
        ▼
Completion
        │
        ▼
Notify Waiting Requests
```

The optimized implementation uses:

- Pending-write tracking
- Write coalescing
- Batch formation
- Background flushing
- Multi-queue dispatch
- Concurrent outstanding operations

# 8. Write Coalescing

The optimized cache maintains a pending-write table indexed by LBA.

## Conceptually:

**Pending Write Table**

| LBA | Latest Value |
|-------|--------------|
| 100   | Latest Value |
| 250   | Latest Value |
| 712   | Latest Value |

If multiple writes target the same LBA before the pending batch is flushed:

- Write A → LBA 100
- Write B → LBA 100
- Write C → LBA 100

the pending table retains the latest value:

```text
LBA 100 → Write C
```

The earlier logical requests remain associated with the pending entry through waiting completion objects.

When the physical write completes, the waiting logical requests can be acknowledged.

This reduces the number of physical storage operations while preserving the write-through completion requirement.

# 9. Batch and Flush Architecture

The optimized cache contains a background flusher thread.

Two conditions can cause flushing:

1. The configured flush interval expires.
2. The number of pending unique LBAs reaches the batch-size trigger.

During a flush, the pending table is extracted as a batch, allowing new writes to continue entering a new pending table while dispatching occurs.

During this process:

- A background thread monitors conditions and triggers flushes accordingly.
- The extracted batch is dispatched to the NVMe device layer for actual storage operations.

# 10. NVMe-Style Device and Queue Architecture

The NVMe device component models an NVMe-style multi-queue architecture in software.

The device maintains multiple queue pairs. Each queue pair has a worker responsible for processing submitted write commands.

Conceptually:

```text
                       NVMe Device
                            │
          ┌─────────────────┼─────────────────┐
          │                 │                 │
          ▼                 ▼                 ▼
       Queue 0           Queue 1           Queue N
          │                 │                 │
          ▼                 ▼                 ▼
       Worker 0           Worker 1           Worker N
          │                 │                 │
          └─────────────────┼─────────────────┘
                            │
                            ▼
                     Storage Backend
```

# 11. Storage Backend

The storage backend performs the actual file I/O used by the simulator.

The implementation uses Linux system calls and mechanisms including:

- `pwrite()`
- `O_DIRECT`
- `posix_memalign()`

An LBA is mapped to a storage offset using the configured block size.

Conceptually:

```text
Storage Offset = LBA × Block Size
```

The backing file provides the storage medium for the software simulation.

# 12. Completion Handling

Physical write requests are represented as asynchronous commands.

The queue worker processes the command and signals completion.

The project uses C++ `std::promise` and `std::future` mechanisms to represent asynchronous completion.

Conceptually:

```text
Application
    │
    ▼
Submit Write
    │
    ▼
Queue
    │
    ▼
Worker Thread
    │
    ▼
pwrite()
    │
    ▼
Completion
    │
    ▼
Promise/Future
    │
    ▼
Waiting Request
```

This allows the optimized cache to maintain multiple outstanding operations.

# 13. Synchronization Mechanisms

The current implementation uses several C++ synchronization mechanisms.

## Mutex

Used to protect shared structures such as:

- Cache
- Pending Write Table
- Submission Queue

## Condition Variable

Used to:

- Wake queue workers
- Wake the optimized cache flusher

## Atomic Variables

Used for state and statistics such as:

- Running
- Round-robin counter
- Queue statistics

## Promise / Future

Used to represent asynchronous completion.

These mechanisms allow multiple threads to safely interact with the cache, queues, and storage layer.

# 14. Component Dependency

The primary dependency relationship is:

```text
Metrics Collector
       │
       ▼
Cache Layer
       │
       ▼
NVMe Device
       │
       ▼
NVMe Queue Pairs
       │
       ▼
Storage Backend
```

The cache layer depends on the device abstraction to submit physical writes.

The device abstraction manages queue pairs.

Queue pairs perform the actual storage operations.

Metrics are collected throughout the execution and summarized after the benchmark.

# 15. Baseline Architecture

The baseline architecture intentionally exposes the bottleneck being studied.

```text
Application --> Baseline Cache --> Queue 0 --> Worker 0 --> pwrite() --> Completion --> Application
```

The caller waits for each operation to complete before continuing.

Therefore, the baseline does not exploit the available software-modeled multi-queue parallelism.

# 16. Optimized Architecture

The optimized architecture introduces a deeper pipeline:

```text
Application Threads
        │
        ▼
Optimized Cache
        │
        ▼
Pending Write Table
        │
        ▼
Write Coalescing
        │
        ▼
Batch Formation
        │
        ▼
NVMe Device
        │
        ├───────────────┬───────────────┐
        ▼               ▼               ▼
     Queue 0          Queue 1         Queue N
        │               │               │
        ▼               ▼               ▼
     Worker 0         Worker 1        Worker N
        │               │               │
        └───────────────┴───────────────┘
                        │
                        ▼
                 Storage Backend
```

This architecture increases the number of outstanding operations and allows multiple queue workers to operate concurrently.

# 17. End-to-End Data Flow

The complete optimized path is:

1. Workload Generator →
2. Application Write →
3. Optimized Cache →
4. Cache Update →
5. Pending Write Table →
6. Coalescing →
7. Batch Trigger / Timer →
8. Batch Extraction →
9. NVMeDevice →
10. Queue Selection →
11. Queue Worker →
12. `pwrite()` `O_DIRECT` →
13. `pwrite()` Completion →
14. Waiting Request Completion →
15. Metrics Collection.

# 18. Architectural Design Principles

The architecture follows these principles:

- **Separation of Concerns:** Each major subsystem has a specific responsibility.
- **Common Device Model:** Both baseline and optimized caches use the same `NVMeDevice` implementation.
- **Controlled Comparison:** The workload and device environment can be kept consistent when comparing implementations.
- **Concurrency:** The optimized design allows multiple operations to remain outstanding.
- **Reproducibility:** Workloads can be configured using deterministic random seeds.
- **Observable Performance:** The system exposes metrics needed to evaluate latency, throughput, IOPS, coalescing and queue utilization.
- **Explicit Simulation Boundary:** The project models NVMe-style queue behavior in software and does not claim to implement a real NVMe controller.

# 19. Current C++ Class Mapping

The current implementation maps to the architecture as follows:

| C++ Class | Architectural Role |
| --- | --- |
| `AlignedBuffer` | Aligned I/O buffer management |
| `WriteCommand` | Physical write command |
| `QueueStats` | Queue performance statistics |
| `NVMeQueuePair` | Software NVMe-style queue pair |
| `NVMeDevice` | Device and multi-queue manager |
| `BaselineWriteThroughCache` | Baseline cache |
| `OptimizedWriteThroughCache` | Optimized cache |
| `PendingEntry` | Pending/coalesced write state |
| `MetricsCollector` | Performance metrics |

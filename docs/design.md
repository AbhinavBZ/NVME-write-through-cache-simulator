//# Detailed Data Flow

## Write-Through Caching NVMe Accelerator Simulator

**Project:** Write-Through Caching NVMe Accelerator Simulator  
**Stage:** Stage 3 — System Design & Architecture  
**Document:** Detailed Data Flow  
**Language:** C++17

---

# 1. Purpose

This document describes how logical write requests move through the Write-Through Caching NVMe Accelerator Simulator.

Two execution paths are considered:

1. Baseline Write-Through Cache
2. Optimized Write-Through Cache

Both paths eventually submit physical writes to the same software-modeled `NVMeDevice`.

The optimized path additionally performs:

- Pending-write tracking
- Write coalescing
- Batch formation
- Background flushing
- Multi-queue dispatch

---

# 2. Overall Data Flow

The overall system data flow is:

```text
                    Workload Generator
                           │
                           ▼
                    Logical Write
                           │
                           ▼
                    Cache Layer
                     /         \
                    /           \
                   ▼             ▼
             Baseline        Optimized
               Path             Path
                 │                │
                 │                ▼
                 │         Pending Write Table
                 │                │
                 │                ▼
                 │            Coalescing
                 │                │
                 │                ▼
                 │             Batching
                 │                │
                 └───────┬────────┘
                         ▼
                   NVMeDevice
                         │
                         ▼
                NVMe Queue Pairs
                         │
                         ▼
                  Storage Backend
                         │
                         ▼
                     Completion
                         │
                         ▼
                     Metrics
# 3. Workload Generation Flow

The benchmark first generates a logical workload.

The workload configuration can include:
- Number of requests
- Address-space size
- Distribution
- Zipf skew
- Random seed
- Application thread count

The supported distributions include:
- Uniform
- Zipf

The generated workload determines the sequence of logical LBAs that will be written.

## Conceptually:
```
Workload Configuration
          │
          ▼
   Workload Generator
          │
          ▼
Logical Write Requests
          │
          ├── LBA
          └── Data
```

The same workload configuration can be used when comparing baseline and optimized implementations.

# 4. Baseline Write Flow

The baseline path is intentionally synchronous.

## The flow is:
```
Application  BaselineWriteThroughCache::write()
      Update In-Memory Cache
      Create AlignedBuffer
      Copy Write Data
      NVMeDevice::submit_write()
      Queue 0
      Queue Worker
      pwrite()
      Completion
      Future Completion
      Record Latency
      Return to Application```
```
### The application thread therefore waits until the physical write has completed.
### The latency between submission and completion is recorded by the metrics collector.

# 5. Baseline Cache Update

When a write arrives, the baseline cache first updates its in-memory cache.
> **Conceptually:**
> ```
cash_[LBA] = value```
# 23. Modeled Timing Mode

When simulated latency is enabled:

- `sim_latency_ns > 0`

the system still performs the actual `pwrite()` operation.

However, the service time reported by the queue model is based on a configured analytical latency with jitter.

## Conceptually:

```
Physical pwrite()
       │
       ▼
Actual Storage Operation

Configured Model
       │
       ▼
Base Latency ± Jitter
       │
       ▼
Modeled Service Time
```

This separates the queueing experiment from variations in the host storage environment.

# 24. Queue Completion

After the physical operation finishes, the queue worker records statistics and fulfills the command's completion promise.

```
pwrite()
   │
   ▼
Completion Timestamp
   │
   ├── Update Queue Statistics
   │
   └── Fulfill Promise
            │
            ▼
          Future
```

The completion timestamp is therefore available to the caller waiting on the future.

# 25. Optimized Request Completion

After all commands in the extracted batch have been submitted, the optimized cache waits for their futures.

For each completed physical write:

```
Physical Completion
       │
       ▼
Find Pending Entry
        	│	        	▼ Complete All Waiters	        	│	        	▼ Logical Requests Acknowledged``` 
defaults to a coalesced write:
to a coalesced write:
to a coalesced write:
to a coalesced write: Write A ─┐Write B ─┼──► One Physical WriteWrite C ─┘          │                    ▼                Completion                    │          ┌─────────┼─────────┐          ▼         ▼         ▼       Waiter A  Waiter B  Waiter C"}

# 26. Metrics Data Flow

Metrics are collected during and after execution.

The data flow is:

- **Logical Requests**
  - Latency
  - Request Count
  - Logical Bytes
    
    ▼

**MetricsCollector**
    
    ▲
    
    │

- **Physical Writes**   &nbsp;&nbsp;**Queue Stats**
  - Busy Time
  - Operations
  - Utilization
    
    │
    
    ▼

**Performance Report**

---

# 27. Logical and Physical Operation Flow

The benchmark distinguishes between:

## Logical Operations
        
        ▼
Application Requests

and:

## Physical Operations
        
        ▼
Actual Storage Commands

### Without coalescing:
- 100 logical writes → 100 physical writes
### With coalescing:
- 100 logical writes → 80 physical writes
*The difference provides the basis for measuring coalescing reduction.*

---


# 28. End-to-End Baseline Flow

Workload Generator → Application Thread → Baseline Cache → Update Cache → Aligned Buffer → NVMeDevice → Queue 0 → Queue Worker → O_DIRECT pwrite() → Completion → Future → Latency Measurement → Application Continues.

---

# 29. End-to-End Optimized Flow

Workload Generator → Application Threads → Optimized Cache → Update Cache → Pending Write Table → Write Coalescing → Batch Trigger / Flush Timer → Batch Extraction → NVMeDevice with multiple queues and workers → O_DIRECT pwrite() → Completion → Waiters → Application Continuation.

---

# 30. Data Ownership

The major relationships include:

- `NVMeDevice` owns the file descriptor and `NVMeQueuePair` objects, which own worker threads.
- `OptimizedWriteThroughCache` owns cache data, the pending write table, and the flusher thread.
- `WriteCommand` holds shared ownership of the aligned write buffer.
- `MetricsCollector` owns recorded latency and metric data.

Further details are covered in the class-design documentation.

---

# 31. Synchronization Points

Synchronization includes:

- Cache access (`cache_mtx_`)
- Pending writes (`pending_mtx_`)
- Queue submission mutexes
- Background flusher condition variable
- Queue worker condition variables
- Promises/futures for command completion

These synchronization mechanisms ensure safe concurrent operation.

---

# 32. Data Flow Summary

The baseline path is simple: one queue and one outstanding operation.

The optimized path involves write coalescing, batching, multiple queues, and concurrent operations.

Both paths use the same device/storage path:

`NVMeDevice -> NVMeQueuePair -> O_DIRECT -> pwrite() -> backing file`

The common storage path supports consistent comparison of the cache strategies under a common testing environment.
---

# 33. Implementation Mapping

The system architecture is implemented through the following source and header files.

| Architectural Component | Source/Header | Responsibility |
|---|---|---|
| Workload Generation | `include/workload.hpp`, `src/workload.cpp` | Generates reproducible logical write requests using Uniform or Zipfian distributions. |
| Application / Benchmark Orchestration | `src/main.cpp` | Parses configuration, creates system components, executes workloads, and coordinates benchmark runs. |
| Baseline Write-Through Cache | `include/write_through_cache.hpp`, `src/write_through_cache.cpp` | Updates the in-memory cache and synchronously submits each write to the device. |
| Optimized Write-Through Cache | `include/write_through_cache.hpp`, `src/write_through_cache.cpp` | Implements pending writes, write coalescing, batching, background flushing, and multi-queue dispatch. |
| NVMe Device Model | `include/nvme_device.hpp`, `src/nvme_device.cpp` | Represents the software NVMe device and manages multiple queue pairs and device-level statistics. |
| NVMe Queue Pair | `include/nvme_device.hpp`, `src/nvme_device.cpp` | Models a submission/completion queue pair and its dedicated worker thread. |
| Write Command | `include/nvme_device.hpp` | Represents a write submitted to a queue, including LBA, block count, buffer, completion, and merge information. |
| Aligned Buffer | `include/nvme_device.hpp`, `src/nvme_device.cpp` | Provides an aligned memory buffer suitable for `O_DIRECT` I/O. |
| Queue Statistics | `include/nvme_device.hpp` | Tracks completed operations, bytes written, busy time, and maximum queue depth. |
| Metrics Collection | `include/metrics.hpp`, `src/metrics.cpp` | Records latency and calculates IOPS, throughput, physical operations, coalescing ratio, and queue utilization. |
| Build System | `Makefile` | Compiles the C++17 source files and produces the simulator executable. |
| Result Visualization | `scripts/plot_results.py` | Processes generated benchmark results and produces performance visualizations. |

## 33.1 Implementation Layers

The implementation can be viewed as the following software layers:

1. **Workload Layer**
   - `WorkloadGenerator`
   - `WorkloadRequest`

2. **Cache Layer**
   - `BaselineWriteThroughCache`
   - `OptimizedWriteThroughCache`

3. **Device Simulation Layer**
   - `NVMeDevice`
   - `NVMeQueuePair`
   - `WriteCommand`
   - `QueueStats`

4. **Linux I/O Layer**
   - `O_DIRECT`
   - `posix_memalign`
   - `pwrite()`
   - Linux file descriptor and backing-file operations

5. **Measurement Layer**
   - `MetricsCollector`
   - benchmark reports
   - queue utilization
   - latency and throughput measurements

6. **Analysis Layer**
   - CSV result generation
   - `scripts/plot_results.py`
   - performance charts

## 33.2 Implementation Sequence

The planned implementation sequence follows the architecture:

```text
Workload Generation
        ↓
Cache Processing
        ↓
Write Coalescing / Batching
        ↓
NVMeDevice
        ↓
NVMeQueuePair
        ↓
WriteCommand
        ↓
Aligned Buffer
        ↓
O_DIRECT + pwrite()
        ↓
Backing Storage File
        ↓
Completion
        ↓
Metrics Collection
        ↓
Result Analysis

```
The baseline and optimized cache implementations share the same device and storage path. This allows their behavior and performance to be compared under a common execution environment.

### 33.3 Implementation Boundary

The project models NVMe-style queueing and parallel I/O in software. It **does not** directly control a physical NVMe controller or implement a production NVMe kernel driver.

The device model uses Linux file I/O as the storage backend while reproducing important concepts such as:
* Queue pairs
* Queue depth
* Command submission and completion
* Parallel workers
* Multi-queue dispatch

> **Note:** This boundary should be maintained consistently in the implementation, documentation, diagrams, and final presentation.
## 34. Development Environment and Tools

### 34.1 Operating Environment

The project is developed and tested in a Linux environment using Ubuntu through WSL2 on Windows.

Linux is used because the project depends on POSIX/Linux system interfaces such as direct file I/O, aligned memory allocation, file descriptors, and multi-threaded execution.

### 34.2 Programming Language

The primary implementation language is:

- C++17

C++ is used for the core simulator because it provides direct control over memory, threads, synchronization, file I/O, and system-level programming concepts.

### 34.3 Compiler

The project uses:

- GCC / G++

The compiler is invoked through the project Makefile with C++17 support and warning flags.

### 34.4 Build System

The project uses:

- GNU Make

The Makefile handles:

- Source compilation
- Object file generation
- Linking
- Build cleanup
- Running the simulator

### 34.5 Linux System Interfaces

The implementation uses Linux/POSIX interfaces including:

- `pwrite()`
- `O_DIRECT`
- `posix_memalign()`
- File descriptors
- POSIX-compatible synchronization primitives

These interfaces provide the low-level storage and memory behavior required by the simulator.

### 34.6 Concurrency

The simulator uses C++ threading and synchronization facilities for:

- Application worker threads
- NVMe-style queue workers
- Background flushing
- Queue synchronization
- Pending-write synchronization

Important synchronization mechanisms include:

- `std::thread`
- `std::mutex`
- `std::condition_variable`
- `std::atomic`
- `std::promise`
- `std::future`

### 34.7 Result Analysis

Python is used for result processing and visualization.

The result-analysis workflow uses:

- Python
- CSV result files
- Matplotlib

The analysis scripts generate performance visualizations such as latency, throughput, IOPS, and comparison charts.

### 34.8 Version Control

Git is used for source-code version control.

GitHub is used as the remote repository for:

- Source code
- Documentation
- Development history
- Project artifacts

The repository follows a normal software-project structure rather than separating files into stage-specific directories.

### 34.9 Hardware Requirements

The simulator does not require a physical NVMe SSD for its core functionality.

The project models NVMe-style queueing and parallel I/O in software while using Linux file I/O as the underlying storage mechanism.

Therefore, the implementation should not be presented as a benchmark of physical NVMe hardware.

### 34.10 Development Tools Summary

| Category | Tool / Technology |
|---|---|
| Operating Environment | Ubuntu on WSL2 |
| Language | C++17 |
| Compiler | GCC / G++ |
| Build System | GNU Make |
| Storage I/O | `pwrite()`, `O_DIRECT` |
| Memory Alignment | `posix_memalign()` |
| Concurrency | C++ Threads |
| Synchronization | Mutex, Condition Variable, Atomic, Promise/Future |
| Analysis | Python |
| Visualization | Matplotlib |
| Version Control | Git |
| Remote Repository | GitHub |
## 35. Git and Development Workflow

### 35.1 Version Control Strategy

Git is used throughout the development lifecycle to maintain a history of implementation and documentation changes.

The repository uses the `master` branch as the primary development branch.

### 35.2 Commit Strategy

Commits are organized around meaningful development changes rather than individual file modifications.

Examples include:

```text
docs: add project requirements and development plan
docs: add system architecture and design
feat: implement optimized write-through cache
feat: add multi-queue device simulation
test: add cache behavior tests
perf: improve write batching
docs: update testing and benchmark results
```
# 35.3 Development Workflow

The general development workflow is:

- **Requirement**
- **Design**
- **Implementation**
- **Build**
- **Test**
- **Debug / Improve**
- **Documentation**
- **Git Commit**
- **GitHub Push**

# 35.4 Branching

The project currently uses the `master` branch as the main development branch.

Additional feature branches may be introduced if the project grows significantly or if larger experimental changes need to be isolated.

# 35.5 Repository Organization

The repository is organized by software responsibility rather than by academic development stage.

The main directories are:

| Directory | Description |
| --- | --- |
| `include/` | C++ header files |
| `src/` | C++ implementation |
| `driver/` | Future Linux driver-related components |
| `tests/` | Automated tests |
| `docs/` | Project documentation |
| `diagrams/` | Architecture and UML diagrams |
| `results/` | Benchmark and experiment outputs |
| `scripts/` | Analysis and visualization scripts |

The academic stages describe the development process and are therefore documented through Git history and project documentation rather than separate stage-1, stage-2, etc., directories.

# 35.6 Documentation Workflow

Documentation is maintained alongside the implementation.

The main technical documents are:

- `docs/requirements.md` — project requirements and development plan
- `docs/architecture.md` — system architecture
- `docs/design.md` — detailed design and implementation mapping
- `docs/testing.md` — testing strategy and results
- `docs/user-guide.md` — build, execution, and usage instructions

The README provides the high-level project overview and entry point for users and evaluators.

# 35.7 Development Traceability

The combination of:

- Requirements 
- Architecture 
- Design documentation 
- Source code 
- Tests 
- Benchmark results 
- Git commits 
p>rovides traceability from the original project objectives to the final implementation.
t>his allows the development process and major technical decisions to be reviewed throughout the project lifecycle.

# 35.8 Stage 3 Development Principle

Stage 3 establishes the technical blueprint before major additional implementation work.

The architecture and design should remain synchronized with the actual implementation. If implementation decisions change during later stages, the corresponding documentation should also be updated.

This prevents the final documentation from describing an architecture that differs from the implemented system.



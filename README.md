# Write-Through Caching NVMe Accelerator Simulator

A Linux/C++17 systems project that investigates how software-level caching
techniques can improve the organization and processing of storage write
requests.

The project compares a conventional synchronous **write-through cache**
against an optimized write-through design using **write coalescing,
batching, and NVMe-style multi-queue processing**.

The storage layer uses real Linux `pwrite()` operations with `O_DIRECT`,
while an optional service-time model allows the architectural behavior of
multi-queue NVMe-style storage to be studied independently of the performance
characteristics of the development machine.

> **Important:** This project is a software NVMe-style storage simulator.
> It does not implement a physical NVMe controller, SSD firmware, or a
> production-grade NVMe kernel driver.

---

## 1. Project Overview

Modern storage systems generate large numbers of I/O requests. The way these
requests are organized, scheduled, and submitted can significantly affect
latency and throughput.

Write-through caching provides strong persistence semantics because data is
propagated to storage before a write is acknowledged. However, a simple
implementation that submits one storage request at a time and waits for its
completion can introduce substantial overhead and may not effectively exploit
the parallelism available in NVMe-style storage systems.

This project explores whether software techniques such as:

- write coalescing
- request batching
- multi-queue processing
- concurrent application threads
- efficient Linux I/O

can improve the behavior of a write-through storage path.

The project is implemented as a **software NVMe-style storage simulator** in
C++17 on Linux.

---

## 2. Problem Statement

A conventional synchronous write-through cache can suffer from high latency
because each logical write may result in an individual physical storage
operation followed by a wait for completion.

Modern NVMe storage architectures provide multiple submission and completion
queues, allowing independent I/O requests to be processed concurrently.
However, a software path that submits one request at a time through a single
queue may fail to exploit this parallelism.

The central problem investigated by this project is:

> **How can write coalescing, batching, and NVMe-style multi-queue
> parallelism reduce the overhead of synchronous write-through caching while
> preserving write-through persistence semantics?**

The project evaluates this question experimentally using controlled
workloads and configurable system parameters.

---

## 3. Motivation

The project is motivated by several storage-system concepts.

### 3.1 Write-through persistence

Write-through caching provides a strong persistence guarantee because the
application does not receive completion until the corresponding data has
been propagated to the storage layer.

### 3.2 Repeated writes

Applications may issue multiple writes to the same logical block within a
short period.

If several pending writes target the same block, earlier values may no
longer need to be physically written when the newer value supersedes them.

This creates an opportunity for **write coalescing**.

### 3.3 Batching

Submitting individual operations can introduce per-request overhead.
Collecting multiple pending operations and processing them together can
increase the amount of useful work performed per scheduling/submission cycle.

### 3.4 Storage parallelism

NVMe devices are designed around multiple queues and concurrent I/O
processing. A software model with multiple independent queue workers allows
this architectural behavior to be studied.

---

## 4. Objectives

The project has the following objectives:

1. Implement a conventional synchronous write-through cache.
2. Implement an optimized write-through cache using write coalescing.
3. Implement configurable batching based on pending-request count and flush
   interval.
4. Model multiple NVMe-style queue pairs.
5. Process independent storage requests concurrently.
6. Use Linux system-programming facilities including:
   - `pwrite()`
   - `O_DIRECT`
   - aligned memory allocation
   - threads
   - mutexes
   - condition variables
   - atomic operations
7. Generate repeatable storage workloads using configurable distributions.
8. Compare the baseline and optimized implementations using common workload
   inputs.
9. Measure latency, throughput, IOPS, queue utilization, and physical
   storage operations.
10. Study how workload locality affects the effectiveness of write coalescing.

---

## 5. Project Scope

### Included

The project covers:

- C++17 systems programming
- Linux system programming
- write-through caching
- write coalescing
- request batching
- NVMe-style multi-queue processing
- concurrent I/O processing
- logical block addressing
- Linux direct I/O
- workload generation
- performance measurement
- CSV-based benchmark output
- reliability testing
- Linux device-driver and storage-system concepts

### Out of Scope

The project does not attempt to implement:

- a physical NVMe controller
- SSD firmware
- NAND flash management
- a Flash Translation Layer
- PCIe protocol implementation
- complete NVMe specification compliance
- FPGA-based storage hardware
- modification of physical SSD firmware
- a production-grade NVMe kernel driver

The NVMe component is therefore explicitly a **software model of
NVMe-style queue-based storage behavior**.

---

## 6. Baseline vs Optimized Approach

The project contains two primary write-through implementations.

| Feature | Baseline | Optimized |
|---|---|---|
| Cache model | Write-through | Write-through |
| Submission model | Synchronous | Batched/background |
| Queue usage | Queue 0 | Multiple queues |
| Queue selection | Fixed | Round-robin |
| Write coalescing | No | Yes |
| Batching | No | Yes |
| Concurrent queue workers | No | Yes |
| Persistence acknowledgment | After storage completion | After corresponding storage completion |

### Baseline

The baseline path represents a simple synchronous implementation:

```text
Application
     |
     v
Write-Through Cache
     |
     v
Queue 0
     |
     v
Linux Storage I/O
     |
     v
Backing Storage
```

Each logical write is submitted individually and waits for completion.

### Optimized

The optimized path introduces additional software-level parallelism:

```text
Application
     |
     v
Optimized Write-Through Cache
     |
     +--> Coalescing
     |
     +--> Batching
     |
     v
NVMe-Style Multi-Queue Layer
     |
     +--> Queue 0
     +--> Queue 1
     +--> Queue 2
     +--> ...
     +--> Queue N
     |
     v
Linux Storage I/O
     |
     v
Backing Storage
```

## 7. Write Coalescing

The optimized cache maintains pending writes indexed by logical block
address.

When multiple pending writes target the same block, the latest value can
replace the earlier pending value.

For example:

```text
Write LBA 100 -> A
Write LBA 100 -> B
Write LBA 100 -> C
```

can become:

```text
Physical write:

LBA 100 -> C
```

instead of three separate physical operations, provided the writes are still
within the coalescing window.

This follows a **last-writer-wins** model.

The application requests are not acknowledged before the corresponding
physical storage operation completes.

## 8. Batching and Multi-Queue Processing

The optimized implementation collects pending writes and flushes them when
configured conditions are reached.

Two important parameters are:

- `flush_interval_us`
- `batch_trigger`

Once a batch is ready, requests are dispatched through the NVMe-style queue
layer.

The simulator models multiple queue pairs, with each queue represented by a
worker responsible for processing submitted storage commands.

This allows the project to study the relationship between:

- concurrency
- queueing
- latency
- throughput
- queue utilization

## 9. Linux System Programming

The project uses Linux I/O facilities rather than implementing storage
operations entirely as an abstract mathematical model.

The storage layer uses:

- `pwrite()`
- `O_DIRECT`
- `posix_memalign()`
- file descriptors

Concurrency uses C++ wrappers around POSIX/Linux threading primitives:

- `std::thread`
- `std::mutex`
- `std::condition_variable`
- `std::atomic`

Completion signaling uses:

- `std::promise`
- `std::future`

The project also uses RAII and STL containers for resource management and
data organization.

## 10. Linux Device Driver Concepts

The project is implemented as a **user-space Linux/C++ simulator**, but it
models several concepts relevant to Linux storage and device-driver
architecture.

| Linux / Storage Concept | Project Implementation |
|---|---|
| Device abstraction | `NVMeDevice` |
| Request submission | `submit_write()` |
| Queue management | `NVMeQueuePair` |
| Request completion | `std::promise` / `std::future` |
| Multi-queue I/O | Multiple queue pairs |
| Queue scheduling | Round-robin dispatch |
| Block I/O | LBA + 4096-byte blocks |
| Linux storage I/O | `pwrite()` |
| Direct I/O | `O_DIRECT` |
| Aligned buffers | `posix_memalign()` |
| Background request processing | Queue worker threads |
| Kernel module | Not implemented |

### Kernel Module Status

A loadable Linux kernel module (`.ko`) has **not** been implemented in the
current version.

The current project focuses on user-space Linux system programming and
simulation of storage concepts such as:

- device abstraction
- request submission
- request completion
- queue management
- multi-queue processing
- block-oriented I/O
- direct Linux I/O

This distinction is important: the project demonstrates and models relevant
storage/device-driver concepts without claiming to be an actual kernel
driver.

## 11. Workload Generation

The simulator supports multiple workload distributions.

### Uniform

Addresses are selected approximately uniformly across the configured address
space.

This produces relatively low locality and therefore provides a useful
workload for examining multi-queue behavior without intentionally creating
high duplicate-write locality.

### Zipf

A Zipf distribution creates skewed access patterns where some blocks are
accessed more frequently than others.

This provides more opportunities for write coalescing.

The workload generator uses the same generated workload parameters for the
baseline and optimized paths so that comparisons remain consistent.

## 12. Performance Metrics

The simulator records metrics including:

- average latency
- p50 latency
- p95 latency
- p99 latency
- maximum latency
- application throughput
- physical throughput
- application IOPS
- physical IOPS
- physical storage operations
- coalescing reduction
- average queue utilization
- per-queue utilization
- wall-clock execution time

Results are written to CSV files for comparison and analysis.

## 13. Real vs Modeled Device Timing

The project supports two timing modes.

### Real Linux I/O

Using:

```bash
./nvme_wt_sim --sim-latency-us 0
```

the simulator relies on the timing of real Linux `O_DIRECT` `pwrite()`
operations against its backing file.

This measures the behavior of the software on the actual development
environment.

### Modeled Service Time

Using a positive value such as:

```bash
./nvme_wt_sim --sim-latency-us 60
```

the simulator introduces a configurable service-time model.

This mode is useful for studying architectural behavior without treating the
performance of the development machine's storage subsystem as equivalent to
the performance of a physical NVMe SSD.

Therefore, modeled results should be interpreted as **simulation results**,
not measurements of a particular physical NVMe device.

## 14. Technology Stack

| Category | Technology |
|---|---|
| Language | C++17 |
| Operating System | Linux |
| Development Environment | Ubuntu on WSL2 |
| Compiler | GNU g++ |
| Build System | GNU Make |
| Storage I/O | `pwrite()` |
| Direct I/O | `O_DIRECT` |
| Memory Alignment | `posix_memalign()` |
| Concurrency | `std::thread` |
| Synchronization | `std::mutex`, `std::condition_variable` |
| Atomic Operations | `std::atomic` |
| Completion | `std::future`, `std::promise` |
| Data Structures | C++ STL |
| Benchmark Output | CSV |
| Version Control | Git |

## 15. Build and Run

### Build

Build the project using GNU Make:

```bash
make
```

### Run Tests

The project includes unit and integration tests:

```bash
make test
```

### Display Available Options

```bash
./nvme_wt_sim --help
```

### Example Workload

```bash
./nvme_wt_sim \
  --requests 1000 \
  --address-space 50 \
  --queues 8 \
  --distribution zipf \
  --zipf-skew 1.5 \
  --app-threads 8 \
  --flush-interval-us 200 \
  --batch-trigger 64 \
  --sim-latency-us 60
```

The program produces a comparison report and writes:

```text
results/comparison.csv
```

### Important Command-Line Parameters

| Parameter | Purpose |
|---|---|
| `--requests` | Number of logical write requests |
| `--address-space` | Number of addressable storage blocks |
| `--queues` | Number of NVMe-style queue pairs |
| `--distribution` | Uniform or Zipf workload |
| `--zipf-skew` | Zipf locality parameter |
| `--flush-interval-us` | Optimized-cache flush interval |
| `--batch-trigger` | Pending writes required to trigger a flush |
| `--app-threads` | Concurrent application threads |
| `--sim-latency-us` | Modeled per-operation service time |
| `--backing-dir` | Directory for backing storage files |
| `--results-dir` | Directory for benchmark output |

## 16. Testing and Validation

The project was tested at multiple levels.

### Unit Testing

Tests cover:

- workload generation
- metrics collection
- cache/device integration
- write coalescing

### Reliability Testing

Additional tests cover:

- minimum workload
- zero-request workload
- single-thread execution
- different queue counts
- different address-space sizes
- Uniform workloads
- Zipf workloads
- different application thread counts
- batch-trigger boundaries
- real Linux `O_DIRECT` I/O
- modeled service-time execution
- invalid queue counts
- invalid address-space values
- invalid Zipf skew
- invalid workload distribution
- edge-case parameter handling

### Coalescing Verification

A deterministic coalescing test verifies that multiple logical writes targeting
the same LBA within the coalescing window can be reduced to fewer physical
writes.

The test demonstrates the relationship between:

```text
Logical writes
      ↓
Pending write table
      ↓
Coalescing
      ↓
Physical writes
```

Detailed test information is available in:

```text
docs/testing.md
docs/reliability-testing.md
```

## 17. Project Documentation

Additional project documentation is available in the `docs/` directory.

| Document | Description |
|---|---|
| `requirements.md` | Project requirements and development plan |
| `architecture.md` | System architecture and component relationships |
| `design.md` | Detailed software design |
| `testing.md` | Testing approach and test evidence |
| `reliability-testing.md` | Reliability and edge-case testing |
| `linux-driver-concepts.md` | Linux device-driver and storage concept mapping |

## 18. Project Structure

```text
nvme_wt_sim/
├── .gitignore
├── Makefile
├── README.md
│
├── include/
│   ├── metrics.hpp
│   ├── nvme_device.hpp
│   ├── workload.hpp
│   └── write_through_cache.hpp
│
├── src/
│   ├── main.cpp
│   ├── metrics.cpp
│   ├── nvme_device.cpp
│   ├── workload.cpp
│   └── write_through_cache.cpp
│
├── tests/
│   ├── cache_integration_test.cpp
│   ├── coalescing_test.cpp
│   ├── metrics_test.cpp
│   └── workload_test.cpp
│
└── docs/
    ├── requirements.md
    ├── architecture.md
    ├── design.md
    ├── testing.md
    ├── reliability-testing.md
    └── linux-driver-concepts.md
```

Generated binaries, object files, logs, CSV files, and charts are excluded
from normal Git tracking through `.gitignore`.

## 19. Development Stages

The project was developed incrementally through six stages.

### Stage 1 — Project Introduction

Defined:

- project idea
- problem statement
- motivation
- objectives
- scope
- expected outcome
- technology stack

**Status: Completed**

### Stage 2 — Requirements and Development Plan

Defined:

- functional requirements
- non-functional requirements
- project requirements
- modules
- features
- constraints
- development plan

**Status: Completed**

### Stage 3 — System Design and Architecture

Developed:

- system architecture
- component responsibilities
- data structures
- class design
- sequence and processing flows
- implementation plan
- development environment
- Git workflow

**Status: Completed**

### Stage 4 — Initial Implementation and Prototype

Implemented and integrated:

- storage layer
- NVMe-style queue layer
- cache layer
- workload generator
- metrics collection
- baseline implementation
- optimized implementation
- write coalescing
- batching
- multi-queue processing

**Status: Completed**

### Stage 5 — Testing, Integration and Improvement

Performed:

- unit testing
- integration testing
- system testing
- performance experiments
- debugging
- reliability testing
- invalid-input testing
- workload comparison
- real Linux I/O testing
- modeled service-time testing

**Status: Completed**

### Stage 6 — Final Implementation and Presentation

Prepared:

- final implementation
- benchmark evidence
- architecture documentation
- testing documentation
- Linux device-driver concept documentation
- source code
- Git history
- project README
- limitations
- demonstration material

**Status: Final Preparation / Submission**

## 20. Current Project Status

| Area | Status |
|---|---|
| Project concept | Complete |
| Requirements | Complete |
| System architecture | Complete |
| Software design | Complete |
| Linux/C++ implementation | Complete |
| Baseline cache | Implemented |
| Optimized cache | Implemented |
| Write coalescing | Implemented |
| Batching | Implemented |
| Multi-queue model | Implemented |
| Workload generation | Implemented |
| Metrics | Implemented |
| Unit testing | Complete |
| Integration testing | Complete |
| Reliability testing | Complete |
| Input validation | Complete |
| Real Linux I/O testing | Verified |
| Modeled service-time testing | Verified |
| Linux driver concept documentation | Complete |
| GitHub repository | Prepared |
| Final demonstration | Preparation |

## 21. Key Observations

The experiments show that the optimized implementation does not necessarily
produce lower latency for every workload.

The effectiveness of the optimized design depends on factors including:

- workload locality
- number of queues
- application concurrency
- batching conditions
- coalescing opportunities
- synchronization overhead
- storage service time

Workloads with greater locality can provide more opportunities for write
coalescing and therefore reduce the number of physical storage operations.

For workloads with limited locality, the additional synchronization,
background flushing, and batching overhead can reduce or eliminate the
benefit.

Therefore, the optimized implementation should not be interpreted as being
universally faster. The purpose of the project is to study how different
storage-system techniques affect behavior under different workloads.

## 22. Limitations

The current implementation has several limitations:

1. The NVMe device is modeled in software rather than implemented as physical
   NVMe hardware.

2. A loadable Linux kernel driver (`.ko`) is not implemented.

3. The project does not implement the complete NVMe specification.

4. The backing storage is a Linux file rather than a physical NVMe namespace.

5. Modeled service-time results are architectural simulation results and not
   physical SSD measurements.

6. Real `O_DIRECT` measurements depend on the development machine and its
   storage subsystem.

7. The simulator is intended as a learning and systems-programming project,
   not a production storage implementation.

## 23. Future Work

Possible future extensions include:

- implementing a lightweight Linux character-device interface where the
  development environment permits it
- exploring kernel-space request handling
- experimenting with different queue scheduling policies
- evaluating additional workload distributions
- exploring different batching strategies
- improving error propagation for failed physical I/O operations
- extending the storage-device model

These are outside the scope of the current submission.

## 24. Conclusion

The **Write-Through Caching NVMe Accelerator Simulator** demonstrates how
software-level storage techniques can be modeled and evaluated using C++17
and Linux system programming.

The project combines:

```text
Write-Through Caching
        +
Write Coalescing
        +
Batching
        +
NVMe-Style Multi-Queue Processing
        +
Linux Direct I/O
        +
Concurrent Request Processing
        +
Performance Measurement
```

The comparison between the baseline and optimized implementations provides
a practical way to study the relationship between logical application writes,
physical storage operations, queueing, concurrency, workload locality, and
system performance.

The project is intentionally scoped as a **software simulator and
systems-programming experiment**, rather than a physical NVMe controller or
complete Linux kernel storage driver.

## 25. Disclaimer

This project is a software simulator and systems-programming experiment.

The NVMe component models concepts such as queue parallelism, request
submission, request completion, batching, coalescing, and concurrent request
processing. It should not be interpreted as an implementation of a physical
NVMe controller or a complete NVMe hardware/firmware stack.

Performance results depend on the selected workload, simulation parameters,
and development environment. Modeled results are intended to study
architectural behavior rather than represent guaranteed performance on a
specific physical SSD.

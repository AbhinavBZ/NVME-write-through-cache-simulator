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
application does not receive completion until the corresponding data has been
propagated to the storage layer.

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
- result visualization

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
Each logical write is submitted individually and waits for completion.

### Optimized

The optimized path introduces additional software-level parallelism:

```
```

```
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

---

## 7. Write Coalescing

The optimized cache maintains pending writes indexed by logical block

address.

When multiple pending writes target the same block, the latest value can

replace the earlier pending value.

For example:

```
```

```
Write LBA 100 -> A
Write LBA 100 -> B
Write LBA 100 -> C
```

can become:

```
```

```
Physical write:

LBA 100 -> C
```

instead of three separate physical operations, provided the writes are still

within the coalescing window.

This follows a **last-writer-wins** model.

The application requests are not acknowledged before the corresponding

physical storage operation completes.

---

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

-  concurrency 
-  queueing 
-  latency 
-  throughput 
-  queue utilization 

---

## 9. Linux System Programming

The project uses Linux I/O facilities rather than implementing storage

operations entirely as an abstract mathematical model.

The storage layer uses:

- `pwrite()` 
- `O_DIRECT` 
- `posix_memalign()` 
-  file descriptors 

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

---

## 10. Workload Generation

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

---

## 11. Performance Metrics

The simulator records metrics including:

-  average latency 
-  p50 latency 
-  p95 latency 
-  p99 latency 
-  maximum latency 
-  application throughput 
-  physical throughput 
-  application IOPS 
-  physical IOPS 
-  physical storage operations 
-  coalescing reduction 
-  average queue utilization 
-  per-queue utilization 
-  wall-clock execution time 

Results are written to CSV files for further analysis and visualization.

---

## 12. Real vs Modeled Device Timing

The project supports two timing modes.

### Real Linux I/O

Using:

```
```

```
--sim-latency-us 0
```

the simulator relies on the timing of real Linux `O_DIRECT` `pwrite()`

operations against its backing file.

This measures the behavior of the software on the actual development

environment.

### Modeled service time

Using a positive value such as:

```
```

```
--sim-latency-us 60
```

the simulator introduces a configurable service-time model.

This mode is useful for studying architectural behavior without treating the

performance of the development machine's storage subsystem as equivalent to

the performance of a physical NVMe SSD.

Therefore, modeled results should be interpreted as **simulation results**,

not measurements of a particular physical NVMe device.

---

## 13. Technology Stack

| Category          | Technology                              |
| ----------------- | --------------------------------------- |
| Language          | C++17                                   |
| Operating System  | Linux / WSL2 development environment    |
| Compiler          | GNU g++                                 |
| Build System      | GNU Make                                |
| Storage I/O       | `pwrite()`                              |
| Direct I/O        | `O_DIRECT`                              |
| Concurrency       | `std::thread`                           |
| Synchronization   | `std::mutex`, `std::condition_variable` |
| Atomic Operations | `std::atomic`                           |
| Completion        | `std::future`, `std::promise`           |
| Data Structures   | C++ STL                                 |
| Benchmark Output  | CSV                                     |
| Visualization     | Python / Matplotlib                     |
| Version Control   | Git                                     |

---

## 14. Build and Run

Build the project:

```
```

```
make
```

Display available options:

```
```

```
./nvme_wt_sim --help
```

Example workload:

```
```

```
./nvme_wt_sim \
  --requests 20000 \
  --address-space 50000 \
  --queues 8 \
  --distribution zipf \
  --zipf-skew 1.2 \
  --app-threads 64 \
  --flush-interval-us 40 \
  --batch-trigger 32 \
  --sim-latency-us 60
```

The program produces a comparison report and writes:

```
```

```
results/comparison.csv
```

Charts can be generated using:

```
```

```
python3 scripts/plot_results.py results/comparison.csv results/chart.png
```

### Important command-line parameters

| Parameter             | Purpose                                    |
| --------------------- | ------------------------------------------ |
| `--requests`          | Number of logical write requests           |
| `--address-space`     | Number of addressable storage blocks       |
| `--queues`            | Number of NVMe-style queue pairs           |
| `--distribution`      | Uniform or Zipf workload                   |
| `--zipf-skew`         | Zipf locality parameter                    |
| `--flush-interval-us` | Optimized-cache flush interval             |
| `--batch-trigger`     | Pending writes required to trigger a flush |
| `--app-threads`       | Concurrent application threads             |
| `--sim-latency-us`    | Modeled per-operation service time         |
| `--backing-dir`       | Directory for backing storage files        |
| `--results-dir`       | Directory for benchmark output             |

---

## 15. Initial Prototype Verification

The project was rebuilt from source in the Linux/WSL2 development environment

using:

```
```

```
C++17
-O2
-Wall
-Wextra
-pthread
```

The executable successfully compiled from:

```
```

```
src/main.cpp
src/metrics.cpp
src/nvme_device.cpp
src/workload.cpp
src/write_through_cache.cpp
```

A small initial test was executed with:

```
```

```
./nvme_wt_sim \
  --requests 1000 \
  --distribution uniform \
  --app-threads 4 \
  --sim-latency-us 60
```

The program successfully executed both the baseline and optimized paths and

generated:

```
```

```
results/comparison.csv
```

The initial test produced no coalescing because the selected uniform workload

did not generate repeated pending writes to the same logical blocks.

The result is treated as an initial verification point rather than a final

performance conclusion. Further experiments will evaluate workload locality,

concurrency, queue count, batching, and service-time parameters.

---

## 16. Project Structure

```
```

```
nvme_wt_sim/
├── .gitignore
├── Makefile
├── README.md
├── include/
│   ├── metrics.hpp
│   ├── nvme_device.hpp
│   ├── workload.hpp
│   └── write_through_cache.hpp
├── src/
│   ├── main.cpp
│   ├── metrics.cpp
│   ├── nvme_device.cpp
│   ├── workload.cpp
│   └── write_through_cache.cpp
├── scripts/
│   └── plot_results.py
└── results/
```

Generated binaries, object files, logs, CSV files, and charts are excluded

from normal Git tracking through `.gitignore` unless selected as final

project artifacts later.

---

## 17. Development Roadmap

The project is being developed incrementally through six development stages.

### Stage 1 — Project Introduction

Define:

-  project idea 
-  problem statement 
-  motivation 
-  objectives 
-  scope 
-  expected outcome 
-  technology stack 

**Current stage.**

### Stage 2 — Requirements and Development Plan

Define:

-  functional requirements 
-  non-functional requirements 
-  project requirements document 
-  modules 
-  features 
-  constraints 
-  development timeline 

### Stage 3 — System Design and Architecture

Develop:

-  system architecture 
-  component responsibilities 
-  data structures 
-  class diagram 
-  sequence diagram 
-  state-machine diagram 
-  implementation plan 
-  development environment 
-  Git workflow 

### Stage 4 — Initial Implementation and Prototype

Progressively implement and integrate:

-  storage layer 
-  NVMe-style queue layer 
-  cache layer 
-  workload generator 
-  metrics 
-  prototype functionality 

### Stage 5 — Testing, Integration and Improvement

Perform:

-  unit testing 
-  integration testing 
-  system testing 
-  performance benchmarking 
-  debugging 
-  reliability testing 
-  performance improvement 

### Stage 6 — Final Implementation and Presentation

Prepare:

-  final implementation 
-  final benchmark results 
-  architecture documentation 
-  testing evidence 
-  source code 
-  Git history 
-  final report 
-  presentation 
-  demonstration 
-  limitations 
-  future work 

---

## 18. Project Status

| Area                  | Status      |
| --------------------- | ----------- |
| Project concept       | Complete    |
| Initial prototype     | Complete    |
| Linux build           | Verified    |
| Baseline cache        | Implemented |
| Optimized cache       | Implemented |
| Coalescing            | Implemented |
| Batching              | Implemented |
| Multi-queue model     | Implemented |
| Workload generation   | Implemented |
| Metrics               | Implemented |
| Initial benchmark     | Verified    |
| Requirements document | Planned     |
| Architecture/UML      | Planned     |
| Formal testing        | Planned     |
| Final optimization    | Planned     |

---

## 19. Disclaimer

This project is a software simulator and systems-programming experiment.

The NVMe component models concepts such as queue parallelism and concurrent

request processing. It should not be interpreted as an implementation of a

physical NVMe controller or a complete NVMe hardware/firmware stack.

Performance results depend on the selected workload, simulation parameters,

and development environment. Modeled results are intended to study

architectural behavior rather than represent guaranteed performance on a

specific physical SSD.

```
```

````

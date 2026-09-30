# Requirements Specification
## Write-Through Caching NVMe Accelerator Simulator

**Project Type:** B.Tech Capstone Project  
**Domain:** Linux System Programming, Storage Systems, C++, Device/IO Simulation  
**Development Environment:** Linux / Ubuntu under WSL2  
**Language:** C++17

---

# 1. Document Purpose

This document defines the requirements and development plan for the **Write-Through Caching NVMe Accelerator Simulator**.

The project is a Linux/C++ software simulator for write-through caching and NVMe-style multi-queue I/O. It evaluates write coalescing, batching, configurable workloads, Linux file I/O, and modeled storage latency.

It establishes what the system must do, quality requirements, scope, modules, deliverables, acceptance criteria, and development roadmap.

---

# 2. Project Overview

## 2.1 Problem Statement

Write-heavy storage workloads can generate a large number of physical write operations. When multiple writes target the same logical block, submitting every pending write independently can create unnecessary storage traffic.

This project investigates a software architecture that combines write-through caching with write coalescing, batching, and multiple I/O queues to study write behavior and performance.

The project provides two implementations:

1. **Baseline Write-Through Cache**
2. **Optimized Write-Through Cache**

Both implementations can be evaluated under controlled workloads and compared using latency, throughput, IOPS, physical operation count, queue utilization, and other metrics.

## 2.2 Motivation

The simulator provides a controlled experimental environment where workloads can be configured, optimization mechanisms can be enabled, and measurable results can be collected.

## 2.3 Main Objective

Design and implement a Linux/C++ software simulator that demonstrates and evaluates write-through caching with:

- write coalescing,
- batching,
- NVMe-style multi-queue processing,
- configurable workloads,
- and measurable performance characteristics.

---

# 3. Objectives

The project shall aim to:

1. Implement a baseline write-through cache.
2. Implement an optimized write-through cache.
3. Implement pending-write coalescing.
4. Implement configurable batch processing.
5. Simulate multiple NVMe-style I/O queues.
6. Use Linux file I/O as the storage backend.
7. Support real and modeled storage timing.
8. Generate configurable workloads.
9. Support Uniform and Zipf workload distributions.
10. Collect detailed performance metrics.
11. Generate machine-readable benchmark results.
12. Compare baseline and optimized implementations.
13. Support reproducible experiments.
14. Maintain modular and maintainable C++ code.
15. Document architecture, implementation, testing, limitations, and results.

---

# 4. Scope

## 4.1 In Scope

- Linux/C++ implementation
- C++17
- Write-through caching
- Baseline cache implementation
- Optimized cache implementation
- Write coalescing
- Batch processing
- Background flushing
- Multiple software I/O queues
- Concurrent queue workers
- Workload generation
- Uniform distribution
- Zipf distribution
- Configurable random seed
- Configurable application threads
- Linux `pwrite()` based storage operations
- Linux `O_DIRECT` storage mode
- Aligned I/O buffers using `posix_memalign()`
- Real storage timing
- Modeled storage latency
- Latency measurement
- IOPS measurement
- Throughput measurement
- Physical operation counting
- Coalescing statistics
- Queue utilization measurement
- CSV/log result generation
- Benchmark comparison
- Reproducible experiments

## 4.2 Out of Scope

The initial project does not claim to implement:

- A real NVMe controller
- A real hardware NVMe driver
- Firmware-level NVMe functionality
- Hardware-level SSD acceleration
- Real PCIe NVMe command submission
- Hardware-specific NVMe performance characterization
- A production storage system
- A production-grade kernel storage driver

A Linux character-device or kernel-module interface may be investigated later if it is required by the academic project and is feasible in the target environment.

---

# 5. Functional Requirements

## FR-01 — Workload Generation

The system shall generate configurable write workloads supporting:

- Number of requests
- Logical address space
- Random seed
- Application thread count
- Address distribution

Supported distributions:

- Uniform
- Zipf

## FR-02 — Write Request Generation

The system shall generate logical write requests containing:

- Request ID
- Logical Block Address (LBA)
- Write size
- Data
- Timing information where applicable

The initial implementation shall use a fixed block size of **4096 bytes**.

## FR-03 — Baseline Write-Through Cache

The system shall provide a baseline write-through cache.

Flow:

1. Receive write request.
2. Update cache state.
3. Submit corresponding physical write.
4. Wait for completion.
5. Complete the application request.

This provides the reference implementation for comparison.

## FR-04 — Optimized Write-Through Cache

The system shall provide an optimized write-through cache using:

- Write coalescing
- Batch processing
- Multi-queue processing
- Background flushing

## FR-05 — Write Coalescing

The system shall combine appropriate pending writes targeting the same LBA.

For example:

```text
Write A → LBA 100
Write B → LBA 100
Write C → LBA 100
```

may result in one final physical write for the pending coalescing window.

The implementation shall use a latest-pending-write-wins model for writes to the same LBA. Coalescing shall not be described as intentionally discarding a write that has already completed persistence.

## FR-06 — Batch Processing

The optimized implementation shall group pending writes into batches.

Configurable parameters include:

- Batch trigger size
- Flush interval

## FR-07 — NVMe-Style Multi-Queue Processing

The system shall simulate multiple independent I/O queues.

The number of queues shall be configurable, and queue workers shall process submitted operations concurrently.

This represents **NVMe-style software queue behavior**, not a physical NVMe controller.

## FR-08 — Storage Backend

The storage backend shall use Linux file I/O mechanisms including:

- `pwrite()`
- `O_DIRECT`
- `posix_memalign()`

## FR-09 — Configurable Device Latency

The simulator shall support real I/O mode and modeled latency mode.

Real I/O example:

```text
--sim-latency-us 0
```

Modeled latency example:

```text
--sim-latency-us 60
```

## FR-10 — Performance Measurement

The system shall collect:

- Average latency
- p50 latency
- p95 latency
- p99 latency
- Maximum latency
- IOPS
- Throughput
- Logical writes
- Physical writes
- Coalescing reduction
- Wall-clock execution time
- Queue utilization

## FR-11 — Result Generation

The system shall generate:

- CSV
- Log files
- PNG charts where applicable

## FR-12 — Configurable Experiments

The command-line interface shall support parameters including:

```text
--requests N
--address-space N
--queues N
--distribution uniform|zipf
--zipf-skew F
--flush-interval-us N
--batch-trigger N
--app-threads N
--seed N
--sim-latency-us N
--backing-dir DIR
--results-dir DIR
```

## FR-13 — Comparison of Implementations

The system shall execute and compare the baseline and optimized implementations under equivalent workload configurations.

## FR-14 — Reproducible Experiments

The system shall support deterministic workload generation using a configurable random seed. Measured execution timing may still vary because of operating-system scheduling and storage behavior.

## FR-15 — Linux Device-Interface Investigation

The project shall investigate whether a Linux character-device or kernel-module component can be integrated if required by the academic course.

Feasibility shall be evaluated during system design before committing to a kernel-module implementation.

---

# 6. Non-Functional Requirements

## NFR-01 — Performance

The simulator should efficiently process large numbers of write requests. Evaluation shall consider IOPS, throughput, latency percentiles, maximum latency, and execution time.

## NFR-02 — Scalability

The system should support increasing:

- Number of requests
- Application threads
- Number of queues
- Logical address space
- Batch size

## NFR-03 — Configurability

Experiment parameters should be configurable without source-code modification.

## NFR-04 — Reproducibility

Experiments should support repeatable workload generation using the same configuration and random seed.

## NFR-05 — Reliability

The simulator should correctly handle concurrent writes, queue synchronization, background flushing, batch formation, completion signaling, and shutdown.

## NFR-06 — Data Integrity

The simulator should preserve the intended write-through semantics. Pending writes may be coalesced according to the defined latest-pending-write model.

## NFR-07 — Concurrency Safety

Shared data structures shall be protected from race conditions, including pending-write maps, queue structures, batch formation, completion signaling, and shutdown state.

## NFR-08 — Portability

The initial target is Linux/Ubuntu with C++17, g++, POSIX/Linux APIs, and pthread support. Development is currently performed using Ubuntu under WSL2.

## NFR-09 — Maintainability

The implementation shall remain modular, with logical separation between workload, cache, NVMe device/queue simulation, metrics, and experiment runner components.

## NFR-10 — Extensibility

The architecture should allow future extensions such as additional workload distributions, cache policies, queue scheduling policies, storage backends, more detailed NVMe behavior, and optional device/kernel interfaces.

## NFR-11 — Observability

The system shall provide logs, CSV metrics, performance summaries, queue utilization, logical versus physical operation counts, and coalescing statistics.

## NFR-12 — Usability

The simulator should be buildable and runnable with:

```bash
make
./nvme_wt_sim --help
./nvme_wt_sim [options]
```

## NFR-13 — Documentation

Documentation shall cover requirements, architecture, implementation, testing, benchmark methodology, results, limitations, and future work.

## NFR-14 — Code Quality

The implementation should use meaningful names, modular classes, appropriate comments, minimal duplication, compiler warnings, consistent structure, and proper resource management.

Compilation shall use warning flags including:

```text
-Wall
-Wextra
```

## NFR-15 — Resource Management

The system shall correctly manage threads, file descriptors, aligned buffers, synchronization primitives, queues, and pending requests.

---

# 7. System Requirements

## 7.1 Hardware

Recommended development environment:

- x86-64 computer
- Multi-core CPU
- 8 GB RAM or more recommended
- Sufficient disk space for source, build artifacts, backing files, and benchmark results

The core simulator does not require a physical NVMe SSD.

## 7.2 Operating System

Primary target:

- Linux / Ubuntu

Current development environment:

- Ubuntu under WSL2 on Windows

Kernel-module/device-driver requirements shall be separately evaluated for WSL2 compatibility.

## 7.3 Compiler and Build Tools

Required:

- GNU g++
- C++17 support
- GNU Make

Verification:

```bash
g++ --version
make --version
```

## 7.4 Linux/POSIX Facilities

The project uses:

- POSIX threads
- File descriptors
- `pwrite()`
- `O_DIRECT`
- `posix_memalign()`
- Mutexes
- Condition variables
- Futures/promises where applicable

## 7.5 Development Tools

Recommended:

- Git
- GitHub
- VS Code
- WSL2
- Linux terminal
- GDB for debugging where required

---

# 8. Project Modules

## 8.1 Workload Generator

Responsibilities:

- Generate requests
- Select LBAs
- Apply workload distribution
- Control random seed
- Produce configurable workloads

## 8.2 Baseline Write-Through Cache

Responsibilities:

- Receive writes
- Update cache state
- Submit physical writes
- Wait for completion

## 8.3 Optimized Write-Through Cache

Responsibilities:

- Receive writes
- Maintain pending writes
- Perform coalescing
- Form batches
- Trigger background flushes
- Signal request completion

## 8.4 NVMe Device Simulator

Responsibilities:

- Represent the software storage device
- Maintain queue pairs
- Accept write commands
- Dispatch commands to queue workers
- Perform storage operations
- Track queue behavior

## 8.5 Storage Backend

Responsibilities:

- Manage backing file
- Perform aligned writes
- Perform `pwrite()`
- Support `O_DIRECT`
- Provide the physical write target

## 8.6 Metrics Engine

Responsibilities:

- Measure request latency
- Calculate percentile latency
- Calculate IOPS
- Calculate throughput
- Count physical operations
- Calculate coalescing reduction
- Track queue utilization
- Produce result data

## 8.7 Benchmark / Experiment Runner

Responsibilities:

- Parse command-line arguments
- Configure experiments
- Run baseline and optimized tests
- Store results
- Produce comparisons

---

# 9. Feature Priorities

## Priority 1 — Core

- Workload generation
- Baseline cache
- Optimized cache
- Storage backend
- Queue simulation
- Metrics
- Benchmark execution

## Priority 2 — Optimization

- Write coalescing
- Batch processing
- Multi-queue execution
- Configurable flushing

## Priority 3 — Evaluation

- Uniform workloads
- Zipf workloads
- Reproducible seeds
- Latency analysis
- Throughput/IOPS analysis
- Physical-write reduction analysis

## Priority 4 — Academic Extension

Investigate:

- Linux character-device interface
- Kernel-module integration
- Additional queue policies
- Additional cache policies

Extensions should not compromise completion of the core simulator.

---

# 10. Scope and Limitations

The project is a **software simulator and experimental system**, not a replacement for a physical NVMe controller.

The project distinguishes between:

1. Real Linux file I/O
2. Modeled storage latency
3. Software-simulated NVMe-style queues

Performance measurements are dependent on the execution environment when real I/O is used.

The project shall not claim that simulated queue behavior represents the exact behavior of a commercial NVMe controller.

Improvements observed in one workload or configuration shall not automatically be generalized to all workloads.

---

# 11. Development Plan

The project follows six academic development stages.

## Stage 1 — Project Introduction

Activities:

- Define problem
- Define motivation
- Define objectives
- Define scope
- Identify expected outcomes
- Establish project documentation

**Status: Completed**

Deliverables:

- Project README
- Initial Git repository
- Initial GitHub repository
- Project overview

## Stage 2 — Requirements & Development Plan

Activities:

- Functional requirements
- Non-functional requirements
- System requirements
- PRD
- Module definition
- Feature priorities
- Scope and limitations
- Development roadmap
- Deliverables definition

**Status: In Progress**

Deliverable:

```text
docs/requirements.md
```

## Stage 3 — System Design & Architecture

Planned:

- Overall architecture
- Component architecture
- Data-flow design
- Class design
- UML class diagram
- Sequence diagrams
- State diagrams where useful
- Data structures
- Threading model
- Queue architecture
- Storage backend design
- Device-driver/kernel feasibility investigation
- Development environment definition
- Git workflow

Expected deliverables:

- Architecture documentation
- UML diagrams
- Design specification
- Implementation plan

## Stage 4 — Initial Implementation & Prototype

Activities:

- Implement core modules
- Integrate workload generator
- Integrate baseline cache
- Integrate optimized cache
- Integrate queue simulator
- Integrate storage backend
- Integrate metrics
- Run initial demonstrations
- Document implementation issues and solutions

Expected deliverables:

- Working prototype
- Source code
- Initial benchmark results
- Demo evidence

## Stage 5 — Testing, Integration & Improvement

Activities:

- Unit testing
- Integration testing
- System testing
- Concurrency testing
- Data-integrity testing
- Performance testing
- Benchmark experiments
- Debugging
- Optimization
- Code-quality improvements
- Documentation updates

Expected deliverables:

- Test plan
- Test results
- Benchmark results
- Performance charts
- Issue/fix records
- Updated documentation

## Stage 6 — Final Implementation & Presentation

Activities:

- Final system integration
- Final testing
- Final benchmark analysis
- Final architecture documentation
- Final UML
- Final source cleanup
- Final README
- Final report
- Final presentation
- Final demonstration

Expected deliverables:

- Final source code
- Final GitHub repository
- Final report
- Final presentation
- UML diagrams
- Test documentation
- Benchmark results
- Final demo

---

# 12. Development Workflow

The project shall use Git for version control.

The repository shall maintain a normal software-project structure rather than creating separate `stage-1`, `stage-2`, etc. directories. Stages represent the development process.

Expected structure as documentation grows:

```text
nvme_wt_sim/
├── README.md
├── Makefile
├── .gitignore
├── include/
├── src/
├── scripts/
├── docs/
│   ├── requirements.md
│   ├── architecture.md
│   ├── testing.md
│   └── benchmark-methodology.md
└── results/
```

Generated benchmark outputs should remain excluded from Git when appropriate.

---

# 13. Git Development Strategy

Use meaningful commits, for example:

```text
docs: add project introduction and initial documentation
docs: add project requirements and development plan
docs: add system architecture and design
feat: implement core simulator modules
test: add validation and benchmark experiments
docs: finalize results and project documentation
```

The exact commit sequence may evolve as development progresses.

---

# 14. Expected Deliverables

## Documentation

- README
- Requirements document
- Architecture document
- UML diagrams
- Testing documentation
- Benchmark methodology
- Results analysis
- Limitations
- Future work

## Software

- C++ source code
- Header files
- Build system
- Workload generator
- Cache implementations
- Queue simulator
- Storage backend
- Metrics system
- Benchmark runner

## Evidence

- Build output
- Program execution
- Test results
- Benchmark CSVs
- Charts
- Screenshots
- Demo evidence
- Git history

## Final Academic Material

- Project report
- Presentation
- Final demonstration

---

# 15. Acceptance Criteria

The project will be considered functionally complete when:

1. The simulator builds successfully on the target Linux environment.
2. The baseline write-through cache executes correctly.
3. The optimized write-through cache executes correctly.
4. Workload generation works for supported distributions.
5. Write coalescing operates according to the defined semantics.
6. Batch processing operates correctly.
7. Multiple software queues process operations correctly.
8. Storage operations complete without unintended data loss.
9. Metrics are collected correctly.
10. Benchmark results can be generated.
11. Baseline and optimized implementations can be compared.
12. Experiments can be reproduced using defined configurations and seeds.
13. Testing demonstrates correct behavior under supported workloads.
14. Documentation describes architecture, implementation, testing, results, limitations, and future work.
15. The final project can be demonstrated using a documented workflow.

---

# 16. Success Criteria

The project will demonstrate:

- A working write-through caching simulator
- A baseline implementation
- An optimized implementation
- Write coalescing
- Batch processing
- NVMe-style software multi-queue processing
- Linux file-I/O integration
- Configurable workloads
- Performance measurement
- Repeatable benchmark methodology
- Engineering documentation
- A traceable development history

The performance evaluation shall report measured results rather than assuming that the optimized implementation will always outperform the baseline.

---

# 17. Future Extensions

Potential future extensions include:

- Linux character-device interface
- Kernel-module integration where technically and academically appropriate
- More detailed NVMe command modeling
- Read caching
- Additional cache replacement policies
- Additional queue scheduling algorithms
- More workload distributions
- Failure injection
- Persistent metadata
- More detailed storage-device modeling
- Hardware NVMe experiments

These are future possibilities and are not required for the core implementation unless the project scope is formally expanded.

---

# 18. Requirement Traceability

Major requirements shall eventually be mapped to implementation and testing evidence.

| Requirement | Implementation | Test/Evidence |
|---|---|---|
| FR-01 Workload generation | Workload module | Workload tests |
| FR-03 Baseline cache | Baseline cache | Functional tests |
| FR-04 Optimized cache | Optimized cache | Integration tests |
| FR-05 Coalescing | Pending-write map | Coalescing tests |
| FR-06 Batching | Batch flusher | Batch tests |
| FR-07 Multi-queue | Queue-pair workers | Concurrency tests |
| FR-08 Storage backend | Linux file I/O | I/O validation |
| FR-10 Metrics | Metrics module | Metric validation |
| FR-13 Comparison | Benchmark runner | Benchmark results |

The traceability matrix will be expanded during Stages 3–5.

---

# 19. Current Project Status

At the beginning of Stage 2:

- Stage 1 documentation is complete.
- The initial C++ prototype exists.
- The project builds successfully.
- The executable runs successfully.
- Baseline and optimized implementations are present.
- Benchmark functionality is present.
- Git repository is initialized.
- Initial commit has been created.
- GitHub repository has been created and the project has been pushed.

Stage 2 is currently being formalized through this requirements specification.

---

# 20. Document Status

**Document:** Requirements Specification  
**Project:** Write-Through Caching NVMe Accelerator Simulator  
**Stage:** Stage 2 — Requirements & Development Plan  
**Status:** Initial requirements baseline  
**Next Stage:** System Design & Architecture

This document is expected to evolve when later design and implementation work reveals requirements that need clarification or refinement.
---

# 21. Product Requirements Document (PRD)

## 21.1 Product Name

**Write-Through Caching NVMe Accelerator Simulator**

## 21.2 Product Description

A Linux/C++ software simulator for studying write-through caching and NVMe-style I/O behavior.

The system provides baseline and optimized write paths and measures the effect of write coalescing, batching, and multi-queue processing under configurable workloads.

## 21.3 Target Users

The primary users are:

- Student/developer conducting storage-system experiments
- Faculty/evaluator reviewing the capstone project
- Developer running controlled performance experiments

## 21.4 Primary User Goal

The user should be able to configure a workload, execute the simulator, and obtain measurable results comparing the baseline and optimized write-through implementations.

## 21.5 Main User Workflow

```text
Configure Experiment
        │
        ▼
Generate Workload
        │
        ▼
Run Baseline
        │
        ▼
Run Optimized
        │
        ▼
Collect Metrics
        │
        ▼
Generate Results
        │
        ▼
Compare Implementations

## 21.6 Inputs

| Input | Purpose |
|---|---|
| Number of requests | Workload size |
| Address space | LBA range |
| Distribution | Uniform / Zipf |
| Zipf skew | Locality control |
| Application threads | Concurrency |
| Number of queues | Multi-queue configuration |
| Batch trigger | Batch formation |
| Flush interval | Background flushing |
| Random seed | Reproducibility |
| Simulated latency | Controlled device timing |
| Backing directory | Storage location |
| Results directory | Output location |

## 21.7 Processing

The system shall:

1. Generate logical write requests.
2. Pass equivalent workloads through the baseline and optimized paths.
3. Update cache state.
4. Coalesce eligible pending writes in the optimized path.
5. Form batches.
6. Dispatch writes through software-simulated queues.
7. Perform storage operations through the Linux backend.
8. Record request and queue metrics.
9. Produce comparison results.

## 21.8 Outputs

The system shall produce:

- Execution summary
- Latency statistics
- IOPS
- Throughput
- Logical write count
- Physical write count
- Coalescing reduction
- Queue utilization
- Wall-clock execution time
- CSV results
- Log files
- Charts where applicable

## 21.9 Core Product Features

| Feature | Priority |
|---|---|
| Workload generation | Must Have |
| Baseline write-through cache | Must Have |
| Optimized write-through cache | Must Have |
| Write coalescing | Must Have |
| Batch processing | Must Have |
| Multi-queue simulation | Must Have |
| Linux storage backend | Must Have |
| Performance metrics | Must Have |
| Benchmark comparison | Must Have |
| Reproducible workloads | Must Have |
| Result generation | Must Have |
| Kernel/device interface | Investigate / Optional |

## 21.10 Product Constraints

The simulator must:

- Run in the target Linux environment.
- Use C++17.
- Avoid requiring physical NVMe hardware for core functionality.
- Clearly distinguish simulated NVMe-style behavior from real NVMe hardware.
- Keep benchmark methodology reproducible.
- Avoid making unsupported claims about real hardware performance.

## 21.11 PRD Acceptance Criteria

The product requirements are satisfied when a user can:

1. Build the simulator.
2. Display available configuration options.
3. Configure a workload.
4. Generate a repeatable workload using a seed.
5. Execute the baseline implementation.
6. Execute the optimized implementation.
7. Exercise coalescing and batching.
8. Use multiple software queues.
9. Collect performance metrics.
10. Generate benchmark results.
11. Compare both implementations.
12. Interpret the resulting measurements using documented methodology.

---

# 22. Module & Feature Definition

## 22.1 Workload Generator

**Purpose:** Generate controlled write workloads.

### Features

- Generate configurable number of requests
- Generate LBA addresses
- Uniform distribution
- Zipf distribution
- Configurable Zipf skew
- Configurable random seed
- Configurable application threads

### Inputs

```text
requests
address-space
distribution
zipf-skew
seed
app-threads
## 22.2 Baseline Write-Through Cache

**Purpose:** Provide the reference implementation.

### Features

- Receive write request
- Update cache
- Submit physical write
- Wait for completion
- Return completion to application

### Flow

```text
Application
    │
    ▼
Baseline Cache
    │
    ▼
Storage Backend
    │
    ▼
Completion

## 22.3 Optimized Write-Through Cache

**Purpose:** Implement the optimization mechanisms being studied.

### Features

- Pending-write tracking
- Write coalescing
- Batch formation
- Background flushing
- Completion notification

### Flow

```text
Application
      │
      ▼
Optimized Cache
      │
      ▼
Pending Writes
      │
      ▼
Coalescing
      │
      ▼
Batch
      │
      ▼
NVMe-style Queues

22.4 Write Coalescer
--------------------

**Purpose:** Reduce redundant physical writes when multiple pending writes target the same LBA.

Example:

```
LBA 100 ← Write A
LBA 100 ← Write B
LBA 100 ← Write C
              │
              ▼
       Coalesced pending write
              │
              ▼
          LBA 100
```

### Features

-   Track pending LBAs
-   Replace older pending data for the same LBA
-   Maintain waiting request completion
-   Count logical versus physical operations

* * * * *

22.5 Batch Manager / Flusher
----------------------------

**Purpose:** Group pending writes before submitting them to the device layer.

### Features

-   Batch trigger size
-   Flush interval
-   Background flushing
-   Batch extraction
-   Batch submission

### Parameters

```
--batch-trigger
--flush-interval-us
```

* * * * *

22.6 NVMe-Style Queue Simulator
-------------------------------

**Purpose:** Model concurrent I/O queues similar in concept to NVMe submission/completion queues.

### Features

-   Configurable queue count
-   Queue selection
-   Queue workers
-   Concurrent processing
-   Queue utilization measurement

### Example

```
              ┌── Queue 0 ──┐
              ├── Queue 1 ──┤
Requests ────►├── Queue 2 ──┤──► Storage
              ├── Queue 3 ──┤
              └── Queue N ──┘
```

This is a **software simulation**, not a real NVMe controller.

* * * * *

22.7 Storage Backend
--------------------

**Purpose:** Provide the actual write target used by the simulator.

### Features

-   Backing file
-   `pwrite()`
-   `O_DIRECT`
-   Aligned memory
-   LBA-to-file-offset conversion
-   Physical write execution

### Current Block Size

```
4096 bytes
```

* * * * *

22.8 Metrics Engine
-------------------

**Purpose:** Measure system behavior.

### Metrics

```
Average latency
p50
p95
p99
Maximum latency
IOPS
Throughput
Logical writes
Physical writes
Coalescing reduction
Wall-clock time
Queue utilization
```

* * * * *

22.9 Benchmark / Experiment Runner
----------------------------------

**Purpose:** Run controlled experiments and compare implementations.

### Features

-   Parse command-line arguments
-   Configure workload
-   Run baseline
-   Run optimized
-   Collect metrics
-   Generate comparison
-   Save results

* * * * *

22.10 Result & Visualization
----------------------------

**Purpose:** Store and present experiment results.

### Outputs

```
CSV
LOG
PNG charts
```

This module becomes particularly important during Stage 5, when final testing and performance evaluation are performed.

* * * * *

23\. Stage 2 Completion Summary
===============================

The following Stage 2 components have now been defined:

| Component | Status |
| --- | --- |
| Functional Requirements | Complete |
| Non-Functional Requirements | Complete |
| System Requirements | Complete |
| Product Requirements Document | Complete |
| Module & Feature Definition | Complete |
| Scope & Limitations | Complete |
| Development Plan | Complete |
| Deliverables | Complete |
| Acceptance Criteria | Complete |
| Success Criteria | Complete |
| Requirement Traceability | Complete |

Stage 2 is now ready for final review and Git version control.

* * * * *

24\. Next Development Stage
===========================

After Stage 2 is committed, the project will proceed to:

**Stage 3 --- System Design & Architecture**

Stage 3 will define:

-   Overall system architecture
-   Component interactions
-   Data flow
-   Class design
-   Data structures
-   Threading model
-   Queue architecture
-   Cache architecture
-   Storage backend architecture
-   UML class diagram
-   Sequence diagrams
-   State diagrams where useful
-   Linux device-driver/kernel feasibility
-   Implementation plan

No major architectural implementation changes should be made until this design stage is documented.

```

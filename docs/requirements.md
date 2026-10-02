# Project Requirements

## Write-Through Caching NVMe Accelerator Simulator

**Project:** Write-Through Caching NVMe Accelerator Simulator
**Stage:** Stage 2 — Requirements and Development Plan
**Language:** C++17
**Platform:** Linux / Ubuntu under WSL2

---

# 1. Purpose

This document defines the requirements and development plan for the Write-Through Caching NVMe Accelerator Simulator.

The project investigates how software-level write-through caching techniques can organize storage write requests more efficiently.

The system compares:

1. A baseline synchronous write-through cache
2. An optimized write-through cache using:
   - Write coalescing
   - Batching
   - Background flushing
   - NVMe-style multi-queue processing
   - Concurrent application threads

The project is implemented as a Linux/C++ software simulator using a backing storage file and Linux file I/O.

---

# 2. Problem Statement

A simple synchronous write-through cache may submit each logical write as an individual physical storage operation and wait for its completion before continuing.

This can result in:

- High per-request overhead
- Limited queue utilization
- Limited storage parallelism
- Repeated physical writes for the same logical block

The project therefore investigates whether write coalescing, batching, and NVMe-style multi-queue processing can reduce unnecessary physical operations and improve the organization of the write path while preserving write-through behavior.

---

# 3. Project Objectives

The project has the following objectives:

1. Implement a conventional synchronous write-through cache.

2. Implement an optimized write-through cache.

3. Implement write coalescing for repeated writes to the same logical block.

4. Implement configurable batching of pending writes.

5. Implement background flushing of pending writes.

6. Model multiple NVMe-style queue pairs.

7. Process independent physical writes concurrently.

8. Generate repeatable storage workloads using configurable distributions.

9. Compare baseline and optimized implementations using the same workload.

10. Measure:
    - Latency
    - Throughput
    - IOPS
    - Physical operation count
    - Queue utilization
    - Coalescing reduction

---

# 4. Project Scope

## 4.1 Included

The project covers:

- C++17 systems programming
- Linux system programming
- Write-through caching
- Write coalescing
- Request batching
- NVMe-style multi-queue processing
- Concurrent I/O processing
- Logical block addressing
- Linux direct I/O
- Workload generation
- Performance measurement
- Automated testing
- Benchmark result generation
- Git/GitHub based development

---

## 4.2 Out of Scope

The project does not attempt to implement:

- A physical NVMe controller
- SSD firmware
- NAND flash management
- A Flash Translation Layer
- PCIe protocol implementation
- Complete NVMe specification compliance
- FPGA-based storage hardware
- Modification of physical SSD firmware
- A production-grade NVMe kernel driver

The NVMe component is a software model of NVMe-style queue-based storage behavior.

---

# 5. Functional Requirements

## FR-01: Workload Generation

The system shall generate logical write requests for benchmarking.

The workload generator shall support:

- Configurable request count
- Configurable address-space size
- Configurable random seed
- Uniform distribution
- Zipfian distribution
- Configurable Zipf skew

---

## FR-02: Logical Write Requests

Each generated request shall contain the information required to perform a logical storage write.

The request shall include:

- Logical block address
- Write data
- Request information required by the benchmark

---

## FR-03: Baseline Write-Through Cache

The system shall provide a baseline write-through cache.

The baseline implementation shall:

1. Receive a logical write.
2. Update the in-memory cache.
3. Create an aligned write buffer.
4. Submit the physical write.
5. Wait for completion.
6. Complete the logical request.

The baseline path shall act as the reference implementation.

---

## FR-04: Optimized Write-Through Cache

The system shall provide an optimized write-through cache.

The optimized implementation shall support:

- Pending-write tracking
- Write coalescing
- Batching
- Background flushing
- Multi-queue dispatch
- Concurrent outstanding writes

---

## FR-05: Write Coalescing

The optimized cache shall identify multiple pending writes targeting the same logical block.

When multiple writes target the same LBA within the pending-write window, the latest pending value shall replace the earlier pending value.

Conceptually:

```text
Write A ─┐
Write B ─┤
Write C ─┼──> Same LBA
Write D ─┘
           │
           ▼
      Coalescing
           │
           ▼
   One Physical Write
### FR-06 — Optimized Write-Through Cache

The system shall provide an optimized write-through cache implementation that improves write processing by using batching, write coalescing, and multi-queue request distribution.

### FR-07 — Write Coalescing

The system shall detect multiple pending writes targeting the same logical block and combine them into a single physical write operation within the configured flush window.

### FR-08 — Batching

The system shall collect pending write requests and process them as batches based on the configured batch trigger and flush interval.

### FR-09 — Multi-Queue Processing

The system shall support multiple NVMe-style queue pairs and distribute write requests across available queues to model parallel I/O processing.

### FR-10 — Workload Generation

The system shall generate configurable workloads with different request counts, address-space sizes, application thread counts, and workload distributions.

### FR-11 — Uniform Workload

The system shall support a uniform workload distribution in which logical block addresses are selected across the configured address space.

### FR-12 — Zipfian Workload

The system shall support a Zipfian workload distribution to model workloads with different levels of data locality.

### FR-13 — Configurable Simulation Latency

The system shall allow the user to configure modeled device service latency for controlled performance experiments.

### FR-14 — Performance Metrics

The system shall collect and report performance metrics including:

- Logical write requests
- Physical write operations
- Average latency
- P50 latency
- P95 latency
- P99 latency
- Maximum latency
- IOPS
- Throughput
- Wall-clock execution time
- Queue utilization
- Coalescing reduction

### FR-15 — Baseline and Optimized Comparison

The system shall execute the same generated workload using both the baseline and optimized cache implementations and provide comparative performance results.

### FR-16 — Result Generation

The system shall generate benchmark results in a structured format suitable for analysis and comparison.

### FR-17 — Command-Line Configuration

The system shall provide command-line options for configuring major simulation parameters, including:

- Number of requests
- Address-space size
- Number of queues
- Workload distribution
- Zipfian skew
- Application threads
- Flush interval
- Batch trigger
- Simulated latency
- Backing storage directory
- Results directory
### FR-18 — Linux-Based Execution

The system shall be implemented and executed in a Linux environment using C/C++ and Linux system programming interfaces.

### FR-19 — Linux File I/O

The system shall use Linux file I/O mechanisms to model storage operations, including `pwrite()` and direct I/O where applicable.

### FR-20 — Configurable Backing Storage

The system shall support a configurable backing storage location for storing the simulated device data.

### FR-21 — Cache and Device Integration

The cache layer shall communicate with the NVMe-style device layer to submit and complete write operations.

### FR-22 — Request Completion

The system shall provide completion handling for submitted write requests so that application-level requests can wait for their corresponding storage operation to complete.

### FR-23 — Testing and Validation

The system shall provide tests for the workload generator, metrics collection, cache/device integration, and write-coalescing behavior.

### FR-24 — Input Validation

The system shall validate important command-line parameters and report invalid configuration values with an appropriate error message.

### FR-25 — Reliability Testing

The system shall be tested using different workloads, queue configurations, concurrency levels, address-space sizes, and simulation parameters to verify correct behavior.

### FR-26 — Performance Comparison

The system shall allow performance characteristics of the baseline and optimized implementations to be compared using identical workload configurations.

### FR-27 — Documentation

The project shall include documentation describing the requirements, architecture, design, implementation, testing, reliability considerations, and Linux-related concepts.

### FR-28 — GitHub Repository

The complete project shall be maintained in a Git repository containing the source code, build configuration, documentation, and required project files.

### FR-29 — Build System

The system shall provide a reproducible build process using a Makefile.

### FR-30 — Project Demonstration

The completed system shall be executable from the Linux command line so that its functionality and performance comparison can be demonstrated during project evaluation.

## Non-Functional Requirements

### NFR-01 — Performance

The system should provide measurable performance metrics for both baseline and optimized write-through cache implementations.

### NFR-02 — Scalability

The system should support configurable numbers of application threads, queue pairs, requests, and logical address-space sizes.

### NFR-03 — Reliability

The system should operate correctly across different workload configurations and handle valid and invalid input parameters appropriately.

### NFR-04 — Portability

The project should use standard C++17 features and Linux system interfaces available in the target Linux development environment.

### NFR-05 — Maintainability

The source code should be organized into separate modules for workload generation, caching, NVMe-style device processing, metrics, and application control.

### NFR-06 — Modularity

Each major component should have a clearly defined responsibility and communicate with other components through well-defined interfaces.

### NFR-07 — Usability

The simulator should provide command-line options that allow users to configure and execute different experiments without modifying the source code.

### NFR-08 — Reproducibility

The same workload and configuration should be executable repeatedly so that baseline and optimized implementations can be compared consistently.

### NFR-09 — Testability

The project should contain automated tests and integration tests for important system components and behaviors.

### NFR-10 — Documentation Quality

The project should provide sufficient documentation for understanding the architecture, implementation, Linux concepts, build process, testing process, and execution procedure.

## System Components

### 1. Workload Generator

Generates logical write requests according to the configured workload distribution and simulation parameters.

### 2. Baseline Write-Through Cache

Implements the basic write-through approach where each application write is synchronously submitted to the simulated storage device.

### 3. Optimized Write-Through Cache

Implements batching, write coalescing, and multi-queue request distribution to reduce unnecessary physical write operations.

### 4. NVMe-Style Device Layer

Models an NVMe-style storage device using multiple queue pairs and worker threads for concurrent request processing.

### 5. Metrics Collector

Collects latency, throughput, IOPS, physical operations, queue utilization, and coalescing statistics.

### 6. Command-Line Interface

Provides configuration options for controlling workload generation, queue configuration, batching, coalescing, and simulation parameters.

### 7. Backing Storage

Provides the Linux file-based storage layer used by the simulator for modeled physical write operations.

---

## Technology Requirements

| Category | Requirement |
|---|---|
| Programming Language | C++ |
| C++ Standard | C++17 |
| Operating System | Linux |
| Compiler | GNU g++ |
| Build System | Make |
| File I/O | Linux `pwrite()` |
| Direct I/O | `O_DIRECT` |
| Memory Alignment | `posix_memalign()` |
| Concurrency | `std::thread` |
| Synchronization | `std::mutex`, `std::condition_variable` |
| Asynchronous Completion | `std::future`, `std::promise` |
| Version Control | Git |
| Repository | GitHub |

---

## Linux Requirements

The project shall be developed and executed in a Linux environment.

The implementation shall use relevant Linux system-programming concepts, including:

- Linux file descriptors
- File operations
- `pwrite()`
- `O_DIRECT`
- `ftruncate()`
- POSIX memory alignment
- Threads
- Synchronization primitives
- Concurrent I/O processing

Linux Device Driver concepts relevant to the project shall also be documented and related to the simulated storage architecture.

---

## Device and Storage Requirements

The simulated storage device shall:

- Maintain a configurable number of NVMe-style queue pairs.
- Process write requests using worker threads.
- Support logical block addressing.
- Use a configurable block size.
- Maintain queue statistics.
- Support modeled service latency.
- Support Linux-backed file storage for physical write operations.

The project shall clearly distinguish between the simulated NVMe-style architecture and an actual physical NVMe controller.

---

## Performance Requirements

The project shall provide measurable results for:

- Logical write operations
- Physical write operations
- Average latency
- P50 latency
- P95 latency
- P99 latency
- Maximum latency
- IOPS
- Throughput
- Wall-clock execution time
- Queue utilization
- Coalescing reduction

Performance experiments shall be performed using identical workload configurations when comparing the baseline and optimized implementations.

## Testing Requirements

The project shall be tested at multiple levels to verify correctness, integration, reliability, and performance.

### Unit Testing

Individual modules shall be tested independently, including:

- Workload generation
- Metrics collection
- Cache behavior
- NVMe-style device operations

### Integration Testing

The interaction between the cache layer and the NVMe-style device layer shall be tested to verify that write requests are correctly submitted and completed.

### Coalescing Testing

The optimized cache shall be tested with workloads containing repeated writes to the same logical block to verify that multiple logical writes can be reduced to fewer physical write operations.

### Multi-Queue Testing

The system shall be tested using different numbers of queue pairs to verify concurrent queue processing and queue utilization.

### Workload Testing

The simulator shall be tested using:

- Uniform workloads
- Zipfian workloads
- Different address-space sizes
- Different request counts
- Different application thread counts

### Boundary Testing

The system shall be tested with boundary and edge-case configurations, including:

- Zero requests
- Minimum request counts
- Single-thread execution
- Single-queue execution
- Multiple queues
- Small address spaces
- Batch-trigger boundaries
- Zero Zipfian skew
- Invalid command-line parameters

### Regression Testing

Previously validated workloads shall be rerun after source-code changes to ensure that existing functionality continues to work correctly.

### Performance Testing

The baseline and optimized implementations shall be executed using identical configurations so that their latency, throughput, IOPS, physical write count, and queue utilization can be compared.

---

## Project Constraints

The project shall follow these constraints:

1. The implementation shall use C or C++ only.
2. The project shall be developed and executed on Linux.
3. Python, Java, or other programming languages shall not be used for the project implementation.
4. The project shall focus on software and hardware-architecture concepts related to storage systems.
5. The NVMe component shall be treated as a software simulation/model rather than a physical NVMe controller implementation.
6. The project shall remain understandable and suitable for academic evaluation.
7. Performance results shall be interpreted according to the selected workload and simulation configuration.
8. The project shall not claim physical NVMe hardware performance based solely on simulator results.

---

## Development Plan

The project shall be developed through the following stages:

### Stage 1 — Project Introduction

- Define project idea
- Define problem statement
- Define motivation
- Define objectives
- Define project scope
- Define expected outcome
- Define technology stack

### Stage 2 — Requirements and Development Plan

- Define functional requirements
- Define non-functional requirements
- Identify system modules
- Define project constraints
- Prepare the requirements document
- Prepare the development plan

### Stage 3 — System Design and Architecture

- Design system architecture
- Define component responsibilities
- Design data structures
- Prepare class diagrams
- Prepare sequence diagrams
- Prepare state-machine diagrams
- Define implementation approach
- Define Git workflow

### Stage 4 — Initial Implementation and Prototype

- Implement storage layer
- Implement NVMe-style queue layer
- Implement baseline cache
- Implement optimized cache
- Implement workload generator
- Implement metrics collection
- Integrate the components
- Verify initial functionality

### Stage 5 — Testing, Integration and Improvement

- Perform unit testing
- Perform integration testing
- Perform system testing
- Perform reliability testing
- Perform performance benchmarking
- Debug identified issues
- Validate optimized behavior
- Perform regression testing

### Stage 6 — Final Implementation and Presentation

- Complete final implementation
- Prepare final benchmark results
- Finalize documentation
- Prepare testing evidence
- Clean the GitHub repository
- Review Git history
- Prepare project demonstration
- Document limitations and future work
- Complete final submission

## Repository Requirements

The GitHub repository shall contain the complete project required for execution, evaluation, and understanding of the system.

The repository shall include:

- Source code
- Header files
- Makefile
- README.md
- Project documentation
- Test source files
- `.gitignore`
- Required configuration files

Generated build artifacts and temporary files should not be included in normal Git tracking.

---

## Documentation Requirements

The project documentation shall provide sufficient information for another user to understand and execute the project.

The documentation shall cover:

- Project overview
- Problem statement
- Objectives
- Scope
- Requirements
- System architecture
- Software design
- Linux concepts
- Implementation details
- Build instructions
- Execution instructions
- Testing procedure
- Reliability testing
- Performance results
- Limitations
- Future work

The README shall provide the primary instructions for building, running, and understanding the project.

---

## Git Requirements

The project shall use Git for version control.

The Git repository shall maintain a meaningful development history showing the progression of the project.

Commits should be organized around logical development activities, such as:

- Project initialization
- Requirements documentation
- Architecture documentation
- Design documentation
- Initial implementation
- Feature implementation
- Testing
- Reliability improvements
- Documentation updates
- Final cleanup

The final repository shall contain the completed project and its associated documentation.

---

## Expected Outcome

The completed project is expected to provide a working Linux/C++ software simulator demonstrating write-through caching concepts and NVMe-style storage architecture.

The system should demonstrate:

- Baseline write-through processing
- Optimized write-through processing
- Write coalescing
- Batch processing
- Multi-queue request handling
- Configurable workloads
- Performance measurement
- Linux system-programming concepts
- Reliability testing
- Comparison between baseline and optimized approaches

The project should also provide sufficient documentation and source code for academic evaluation and demonstration.

---

## Project Limitations

The project is a software simulator and is not intended to implement a complete physical NVMe controller or hardware/firmware stack.

The simulated NVMe-style queue system models concepts such as:

- Multiple submission/processing queues
- Concurrent request handling
- Queue distribution
- Storage service latency

Performance results depend on:

- Workload characteristics
- Number of application threads
- Number of queues
- Address-space size
- Batch configuration
- Flush interval
- Simulated service latency
- Linux execution environment

Therefore, simulator results should be interpreted as experimental observations of the implemented architecture rather than guaranteed physical SSD performance.

---

## Requirement Traceability

The project requirements shall be traceable to the corresponding implementation and documentation components.

| Requirement Area | Related Component |
|---|---|
| Workload generation | `workload.hpp`, `workload.cpp` |
| Baseline caching | `write_through_cache.hpp`, `write_through_cache.cpp` |
| Optimized caching | `write_through_cache.hpp`, `write_through_cache.cpp` |
| NVMe-style queues | `nvme_device.hpp`, `nvme_device.cpp` |
| Performance metrics | `metrics.hpp`, `metrics.cpp` |
| Command-line configuration | `main.cpp` |
| Automated testing | `tests/` |
| Linux concepts | `docs/linux-driver-concepts.md` |
| Architecture | `docs/architecture.md` |
| Design | `docs/design.md` |
| Testing | `docs/testing.md` |
| Reliability testing | `docs/reliability-testing.md` |
| Project execution | `README.md` |

---

## Requirement Completion Summary

| Requirement Category | Status |
|---|---|
| Functional Requirements | Complete |
| Non-Functional Requirements | Complete |
| System Architecture | Complete |
| Linux System Programming | Implemented |
| NVMe-Style Queue Model | Implemented |
| Baseline Cache | Implemented |
| Optimized Cache | Implemented |
| Write Coalescing | Implemented |
| Batching | Implemented |
| Multi-Queue Processing | Implemented |
| Workload Generation | Implemented |
| Metrics Collection | Implemented |
| Automated Testing | Implemented |
| Reliability Testing | Completed |
| Documentation | Complete |
| GitHub Repository | Complete |
| Final Demonstration | Required |
| Final Submission | Required |

---

## Conclusion

The requirements define a Linux/C++ software simulator for studying write-through caching and NVMe-style storage architecture.

The project focuses on understandable system-programming concepts including caching, write coalescing, batching, concurrent queue processing, Linux file I/O, synchronization, workload generation, and performance measurement.

The completed implementation and documentation shall provide a suitable basis for final testing, demonstration, and academic evaluation.

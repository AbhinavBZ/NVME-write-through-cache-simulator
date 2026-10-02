# Testing and Prototype Results

## 1. Purpose

This document describes the testing performed during Stage 4 of the
Write-Through Caching NVMe Accelerator Simulator.

The purpose of testing is to verify:

- Correctness of workload generation
- Correctness of metrics collection
- Integration between the cache and NVMe device model
- Correctness of write coalescing
- Correctness of the optimized write-through path
- Behavior under different workload distributions
- Performance characteristics under modeled and real Linux I/O timing

Functional tests are used to verify correctness, while performance
experiments are used to characterize system behavior.

---

## 2. Test Environment

The prototype was developed and tested in the following environment.

| Component | Configuration |
|---|---|
| Operating System | Ubuntu on WSL2 |
| Language | C++17 |
| Compiler | GCC / G++ |
| Build System | GNU Make |
| Storage Interface | Linux `pwrite()` |
| Direct I/O | `O_DIRECT` |
| Memory Alignment | `posix_memalign()` |
| Concurrency | C++ threads |
| Synchronization | mutexes, condition variables, futures/promises |
| Workload Generator | Uniform and Zipfian |
| Visualization | Python / Matplotlib |
| Version Control | Git / GitHub |

The simulator uses a backing storage file to model persistent storage.

The project does not claim to benchmark a physical NVMe controller.
The NVMe behavior is modeled in software while the actual storage path
uses Linux file I/O.

---

## 3. Test Strategy

Testing is divided into several levels.

### 3.1 Unit Testing

Individual modules are tested independently.

Examples:

- Workload generation
- Metrics calculation

### 3.2 Integration Testing

Multiple modules are tested together.

Examples:

- Cache + NVMe device
- Optimized cache + queue model
- Write submission + backing storage

### 3.3 Functional Testing

Functional tests verify important project behavior such as:

- Data is written successfully
- Logical writes are completed
- Physical writes are issued
- Coalescing reduces duplicate physical writes
- Final data remains valid

### 3.4 Performance Testing

Performance experiments are used to observe:

- Latency
- IOPS
- Throughput
- Physical operation count
- Queue utilization
- Coalescing reduction

Performance results are workload-dependent and are not treated as
universal hardware performance measurements.

---

# 4. Automated Test Suite

The project currently contains four automated tests.

```text
tests/
├── workload_test.cpp
├── metrics_test.cpp
├── cache_integration_test.cpp
└── coalescing_test.cpp```

# 5. Workload Generator Test
## Objective
Verify that the workload generator produces valid requests for both supported distributions.

## Tests Performed
The test verifies:
- Correct number of generated requests
- Valid LBA values
- Positive block counts
- Uniform workload generation
- Zipfian workload generation
- Deterministic output when the same random seed is used

## Result
**Workload generator tests: PASS**

The test confirms that the workload generator produces structurally valid and reproducible workloads.

# 6. Metrics Collector Test
## Objective
Verify the correctness of latency statistics and performance metrics.

## Metrics Tested
| Metric | Description |
|---------|--------------|
| Logical request count | Count of logical requests |
| Physical operation count | Count of physical operations |
| Average latency | Mean latency across requests |
| Percentile latency | Latency at specific percentiles |
| Maximum latency | Highest observed latency |
| IOPS | Input/output operations per second |
| Throughput | Data transfer rate |

## Result
**Metrics collector tests: PASS**

The test confirms that the metrics collector correctly processes the recorded measurements.

# 7. Cache and Device Integration Test
## Objective
Verify the interaction between the write-through cache and the software NVMe device model.

The test exercises:
> Application → Write-Through Cache → NVMe Device Model → NVMe Queue → Linux pwrite() → Backing Storage File 
Both baseline and optimized cache paths are tested.

## Verification
The test verifies that:
- Writes complete successfully;
- The cache and device model integrate correctly;
- Data reaches the backing storage;
- Both cache implementations can perform writes.

## Result
**Cache/device integration tests: PASS**

# 8. Write Coalescing Test
## Objective
Verify that multiple concurrent writes targeting the same logical block can be coalesced into a smaller number of physical writes.

the test launches eight concurrent writes targeting the same LBA.
> Conceptually:
> - Write A ─┐
> - Write B ─┤
> - Write C ─┤
> - Write D ─┤
> - Write E ─┤──→ Pending Write Table \
't> - Write F ─┤
't> - Write G ─┤
't> - Write H ─┘
t> 	↓
t> 	Coalescing 
t> 	↓
t> 	Physical Write


# These mechanisms introduce overhead.

Therefore:

**Optimization benefit**
- workload locality
- sufficient concurrency
- enough batching opportunity
- minus optimization overhead

Workloads with little address reuse may provide limited coalescing benefit.

Workloads with high address locality provide more opportunities to reduce physical operations.

## 17. Issues Encountered and Resolutions
### 17.1 Optimized Path Was Slower on Small Workloads
**Observation:**
Small workloads sometimes showed higher optimized latency.

**Cause:**
The optimized path introduces background flushing, synchronization, and batching overhead.

**Resolution:**
The behavior was retained because it reflects a legitimate trade-off of the design rather than a functional error.
Larger and more locality-heavy workloads were added to evaluate the optimization under conditions where it has more opportunity to help.

### 17.2 Coalescing Was Difficult to Observe Reliably
**Observation:**
Random workloads did not always produce visible coalescing.

**Cause:**
Coalescing requires multiple writes to the same LBA to overlap within the pending-write window.

**Resolution:**
A deterministic concurrency test was added that intentionally sends multiple writes to the same LBA.
The test verifies the mechanism independently from random workload behavior.

### 17.3 Generated Test Binaries Appeared in Git
**Observation:**
Compiled test executables appeared as untracked files.

**Resolution:**
the repository `.gitignore` was updated with:
tests/*_test
This prevents generated test binaries from being committed.

### 17.4 Metrics Test Assumptions
**Observation:**
An initial metrics test assumed a specific coalescing-ratio interface that was not exposed by the current public API.

**Resolution:**
the test was corrected to validate the metrics that are actually provided by the MetricsCollector public interface.
This keeps the tests aligned with the implemented API.

## 18. Current Prototype Status
The following Stage 4 components are currently implemented and tested:
| Component | Status |
|---|---|
| Project build | Complete |
| Workload generator | Complete |
| Uniform workload | Complete |
| Zipf workload | Complete |
| Metrics collector | Complete |
| NVMe queue model | Complete |
| Baseline cache | Complete |
| Optimized cache | Complete |
| Write batching | Complete |
| Write coalescing | Complete |
| Multi-queue scheduling | Complete |
dLinux O_DIRECT path | Complete |
eUnit tests | Complete |
eIntegration tests | Complete |
eCoalescing correctness test | Complete |
ePrototype performance experiments | Complete |
eTesting documentation | Complete |
def The Linux kernel-driver component has not yet been implemented as a `.ko` module. The current prototype is a user-space Linux/C++ software simulator using Linux storage APIs.
def The current prototype is a user-space Linux/C++ software simulator using Linux storage APIs.
dThe Linux kernel-driver component has not yet been implemented as a `.ko` module. The current prototype is a user-space Linux/C++ software simulator using Linux storage APIs.
dThe Linux kernel-driver component has not yet been implemented as a `.ko` module. The current prototype is a user-space Linux/C++ software simulator using Linux storage APIs.

# 19. Stage 4 Limitations

The current prototype has several limitations.

## 19.1 Software NVMe Model

The queue and NVMe behavior are modeled in software rather than implemented using a real NVMe controller interface.

## 19.2 Backing Storage

The device model uses a backing file rather than directly managing a physical NVMe namespace.

## 19.3 WSL2 Environment

Testing has been performed under Ubuntu on WSL2.

Kernel-level driver development and loading have not been demonstrated in this environment.

## 19.4 Workload Scope

The current workload generator focuses on write requests and supports uniform and Zipfian address distributions.

## 19.5 Parameter Tuning

Batch size, flush interval, queue count, application concurrency, and workload locality can significantly affect the results.

# 20. Stage 4 Conclusion

Stage 4 established a working prototype of the proposed Write-Through Caching NVMe Accelerator Simulator.

The implementation now includes:
- Workload Generation 071;
- Application Threads 071;
- Write-Through Cache ;
- Baseline ; 
- Optimized ;
- Coalescing + Batching ;
- Multi-Queue Model ;
- Linux O_DIRECT ;
- Backing Storage
;

The automated test suite passes, including the dedicated coalescing correctness test.

Performance experiments demonstrate that workload locality can create significant opportunities for physical-write reduction.

The prototype is therefore ready for the Stage 5 focus on more systematic testing, integration, reliability, performance analysis, and improvement.

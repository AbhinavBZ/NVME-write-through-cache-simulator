# Reliability and Testing

## 1. Purpose

This document describes the reliability testing performed on the
Write-Through Caching NVMe Accelerator Simulator.

The objective was to verify:

- Correctness of baseline and optimized write-through behavior
- Command-line input validation
- Workload generation
- Write coalescing
- Batch processing
- Multi-queue operation
- Boundary conditions
- Real Linux `O_DIRECT` execution
- Performance behavior under different workloads

The testing focuses on software correctness and simulator behavior.
The project does not claim to reproduce the performance of a physical
NVMe SSD.

---

## 2. Testing Approach

Testing was performed at multiple levels:

1. Unit-level testing
2. Cache/device integration testing
3. Workload-generation testing
4. Input-validation testing
5. Boundary testing
6. Coalescing testing
7. Multi-queue testing
8. Batch-processing testing
9. Performance testing
10. Real `O_DIRECT` testing

The same generated workload is used for both the baseline and optimized
implementations so that their results can be compared under equivalent
conditions.

---

## 3. Functional Test Results

| Test | Configuration / Purpose | Result |
|---|---|---|
| Workload generator | Uniform and Zipf workloads | PASS |
| Metrics collector | Metric calculation and reporting | PASS |
| Cache/device integration | End-to-end cache and device interaction | PASS |
| Coalescing | Multiple writes to same LBA | PASS |
| Minimum workload | 1 request | PASS |
| Zero requests | Empty workload | PASS |
| Single-thread workload | 100 requests, 1 application thread | PASS |
| Multi-queue execution | 8 queues | PASS |
| Batch boundary | 64-request workload | PASS |
| Partial batch | 32-request workload | PASS |
| Uniform distribution | Valid workload generation | PASS |
| Zipf distribution | Valid workload generation | PASS |
| Invalid distribution | Rejected correctly | PASS |

---

## 4. Input Validation Testing

Several invalid command-line configurations were deliberately tested.

### Invalid queue count

Command:

```text
--queues 0```

# Validation and Testing Summary

## Initial Validation
Initially, this caused an invalid runtime condition. Validation was added so that the simulator now terminates cleanly with an error message.

### Expected behavior:
- Error: `--queues` must be greater than 0

### Result:
- PASS

### Invalid address space
Command:
- `--address-space 0`

Initially, this could reach the I/O layer and produce file/I/O errors. Validation was added before workload execution.

### Expected behavior:
- Error: `--address-space` must be greater than 0

### Result:
- PASS

### Invalid Zipf skew
Command:
- `--zipf-skew -1`

Negative skew is not meaningful for the current workload generator. Validation was added.

### Expected behavior:
- Error: `--zipf-skew` must be greater than or equal to 0

### Result:
- PASS

### Invalid distribution
Command:
- `--distribution banana`
The parser was updated so that unsupported distribution names are rejected instead of silently selecting another distribution.

### Expected behavior:
- Error: `--distribution` must be uniform or zipf

### Result:
- PASS

## 5. Boundary Testing 
**Zero requests**
Configuration:
- `--requests 0`
The simulator completed successfully and reported zero logical and physical writes.
Result: PASS This behavior is intentionally supported because an empty workload is a valid boundary case.
**Zero application threads**
Configuration:
- `--app-threads 0`
The implementation normalizes the application thread count to one.
Result: PASS This behavior is intentionally retained rather than treating zero as a fatal configuration error.
**Zero Zipf skew**
Configuration:
- `--distribution zipf`
- `--zipf-skew 0`
The simulator completed successfully.
Result: PASS No significant locality-driven coalescing was expected from this configuration.

## 6. Coalescing Test 
a targeted coalescing test was performed using multiple concurrent writes to the same LBA.
test result:
lLogical writes: 8,
pPhysical writes: 1,
tCoalescing test: PASS This corresponds to an 87.5% reduction in physical writes for this deliberately constructed workload.The result demonstrates that multiple pending writes targeting the same block can be combined before being submitted to the simulated NVMe device.The optimized cache uses a pending-write map keyed by LBA. Within the coalescing window, the latest pending write replaces earlier pending writes to the same LBA.Therefore, the simulator models a last-writer-wins behavior within the coalescing window.
'the optimized implementation supports multiple modeled NVMe queue pairs.A representative configuration used:'
dRequests:       1000
dAddress space:  100
dApplication threads: 8
dQueues:         8
dDistribution:   Uniform
dSimulated latency: 60 usThe optimized implementation distributed physical writes across multiple queues.Observed queue utilization was approximately balanced across the eight queues.This verifies that the round-robin queue-selection mechanism is operating as intended.
'the optimized cache uses a background flusher and a batch trigger.Two important cases were tested.'
defPartial batch Configuration Requests: 32 Batch trigger: 64 The workload contains fewer requests than the batch trigger.Result Baseline physical writes:    32 Optimized physical writes:   27 Physical write reduction:    15.62%Result: PASS The optimized cache correctly flushes pending writes even when the batch does not reach the trigger size.Exact batch boundary Configuration Requests: 64 Batch trigger: 64 Result Baseline physical writes:** **64** **Optimized physical writes:** **56** Physical write reduction:** **12.50%** Result:** **PASS**This verifies that reaching the configured batch threshold does not leave pending writes unprocessed.
'the representative results include metrics such as logical and physical writes, latency, and IOPS for different workloads demonstrating various locality patterns.'
the performance tests show significant reductions in physical writes and latency improvements under localized workloads but also highlight potential overheads for workloads with little locality.'}"}] }```
# 16. Overall Reliability Assessment

The implemented simulator successfully passed:

- Core workload-generation tests
- Metrics tests
- Cache/device integration tests
- Coalescing tests
- Boundary tests
- Invalid-input tests
- Multi-queue tests
- Batch-processing tests
- Regression tests
- Real `O_DIRECT` execution tests

The testing also identified an important design characteristic:

> Optimization effectiveness depends on workload locality and system overhead rather than being guaranteed for every workload.

This is an important result of the project because the purpose of the simulator is to compare two write-through approaches under controlled workloads.

# 17. Conclusion

The reliability and performance testing provides evidence that the baseline and optimized write-through cache implementations operate correctly across normal, boundary, and deliberately challenging workloads.

The optimized implementation successfully demonstrates:
- Write coalescing
- Batch processing
- Multi-queue dispatch
- Reduced physical write operations
- Workload-dependent performance changes

The tests also establish the limitations of the simulator and distinguish software-model behavior from measurements of real NVMe hardware.

The implementation is therefore considered stable enough for the next project phase: final documentation, diagrams, demonstration preparation, and evaluation of the Linux device-driver requirement.

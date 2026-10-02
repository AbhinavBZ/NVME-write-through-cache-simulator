# Reliability and Testing Documentation

## Write-Through Caching NVMe Accelerator Simulator

**Project:** Write-Through Caching NVMe Accelerator Simulator
**Stage:** Stage 5 — Testing, Integration and Improvement
**Language:** C++17
**Platform:** Linux / Ubuntu on WSL2

---

# 1. Purpose

This document records the reliability, edge-case, regression, and integration testing performed on the Write-Through Caching NVMe Accelerator Simulator.

The objective is to verify that the simulator:

- Builds correctly
- Handles valid workloads correctly
- Handles edge-case workloads safely
- Rejects invalid command-line parameters
- Completes baseline and optimized write paths
- Maintains correct logical and physical write accounting
- Performs write coalescing correctly
- Handles different queue and concurrency configurations
- Produces stable results across representative workloads

The testing focuses on software reliability rather than physical NVMe hardware validation.

---

# 2. Reliability Testing Strategy

Reliability testing was performed at several levels.

## 2.1 Build Validation

The complete project is rebuilt from source using the Linux build environment.

```bash
make clean
make
```

The build must complete without compilation errors.

---

## 2.2 Automated Tests

The existing automated test suite is executed using:

```bash
make test
```

The suite includes:

- Workload generator tests
- Metrics collector tests
- Cache/device integration tests
- Write coalescing tests

Expected result:

```text
Workload generator tests: PASS
Metrics collector tests: PASS
Cache/device integration tests: PASS
Coalescing test: PASS
```

---

## 2.3 Edge-Case Testing

The simulator was tested with:

- Minimum request counts
- Zero requests
- Single application thread
- Multiple application threads
- Single queue
- Multiple queues
- Very small address spaces
- High address locality
- Uniform workloads
- Zipfian workloads
- Invalid queue counts
- Invalid address-space values
- Invalid Zipf skew values
- Batch-trigger boundary conditions

---

# 3. Automated Test Suite

The automated tests are located under:

```text
tests/
├── workload_test.cpp
├── metrics_test.cpp
├── cache_integration_test.cpp
└── coalescing_test.cpp
```

The tests are compiled and executed through the project Makefile.

---

# 4. Minimum Workload Test

## Configuration

```text
Requests:       1
Address Space:  32
Distribution:   Uniform
Application Threads: 1
Queues:         1
Simulated Latency: 60 us
```

## Objective

Verify that the simulator can successfully process the smallest practical workload.

## Result

The baseline and optimized implementations both completed successfully.

```text
Baseline:
Logical Writes:  1
Physical Writes: 1

Optimized:
Logical Writes:  1
Physical Writes: 1
```

The test passed.

---

# 5. Zero-Request Test

## Configuration

```text
Requests: 0
```

## Objective

Verify that the simulator handles an empty workload without crashing or producing invalid statistics.

## Result

The simulator completed successfully and produced zero logical and physical operations.

```text
Logical Writes:  0
Physical Writes: 0
```

The test passed.

---

# 6. Single-Thread Test

## Configuration

```text
Requests:       100
Address Space:  100
Distribution:   Uniform
Application Threads: 1
Queues:         1
Simulated Latency: 60 us
```

## Objective

Verify correct behavior when the application generates requests from a single thread.

## Result

Both implementations completed all logical requests successfully.

The baseline produced one physical operation for each logical request.

The optimized path also completed successfully.

The test passed.

---

# 7. Single-Queue Test

## Configuration

```text
Requests:       1000
Address Space:  100
Distribution:   Uniform
Application Threads: 8
Queues:         1
Simulated Latency: 60 us
```

## Objective

Verify that the simulator works correctly when only one NVMe-style queue is configured.

## Result

Both implementations completed all logical requests successfully.

The optimized implementation continued to perform coalescing and batching even though only one queue was available.

The test passed.

---

# 8. Multi-Queue Test

## Configuration

```text
Requests:       1000
Address Space:  100
Distribution:   Uniform
Application Threads: 8
Queues:         8
Simulated Latency: 60 us
```

## Objective

Verify multi-queue processing and queue distribution.

## Result

The baseline completed:

```text
Logical Writes:  1000
Physical Writes: 1000
```

The optimized implementation completed:

```text
Logical Writes:  1000
Physical Writes: 965
```

The optimized path produced:

```text
Physical-write reduction: 3.50%
```

The queue utilization was distributed across the available queues.

The test passed.

---

# 9. Zipfian Locality Test

## Configuration

```text
Requests:       1000
Address Space:  10
Distribution:   Zipf
Zipf Skew:      1.5
Application Threads: 8
Queues:         8
Simulated Latency: 60 us
```

## Objective

Verify the effect of high address locality on write coalescing.

## Result

The baseline performed:

```text
Physical Writes: 1000
```

The optimized implementation performed:

```text
Physical Writes: 470
```

Physical-write reduction:

```text
53.00%
```

The optimized path completed successfully.

This test demonstrates that workloads with high address locality provide more opportunities for write coalescing.

---

# 10. Single-LBA Coalescing Stress Test

## Configuration

```text
Requests:       1000
Address Space:  1
Distribution:   Uniform
Application Threads: 8
Queues:         8
Simulated Latency: 60 us
```

## Objective

Create an intentionally high-locality workload where every request targets the same logical block.

## Result

The optimized implementation reduced the number of physical operations substantially.

```text
Logical Writes:  1000
Physical Writes: 126
```

Physical-write reduction:

```text
87.40%
```

This test provides a strong functional demonstration of the coalescing mechanism.

---

# 11. Zipfian Medium-Locality Test

## Configuration

```text
Requests:       1000
Address Space:  50
Distribution:   Zipf
Zipf Skew:      1.5
Application Threads: 8
Queues:         8
Simulated Latency: 60 us
```

## Result

The optimized implementation produced:

```text
Logical Writes:  1000
Physical Writes: 573
```

Physical-write reduction:

```text
42.70%
```

The test completed successfully.

---

# 12. Batch Trigger Boundary Test

## Configuration

```text
Requests:       32
Address Space:  16
Distribution:   Uniform
Application Threads: 8
Queues:         4
Batch Trigger:  64
Simulated Latency: 60 us
```

## Objective

Verify correct flushing when the number of pending requests remains below the configured batch trigger.

## Result

The optimized implementation completed:

```text
Logical Writes:  32
Physical Writes: 27
```

Physical-write reduction:

```text
15.62%
```

The test passed.

---

# 13. Exact Batch Boundary Test

## Configuration

```text
Requests:       64
Address Space:  32
Distribution:   Uniform
Application Threads: 8
Queues:         4
Batch Trigger:  64
Simulated Latency: 60 us
```

## Objective

Test behavior when the workload reaches the exact configured batch-trigger boundary.

## Result

The optimized implementation completed:

```text
Logical Writes:  64
Physical Writes: 56
```

Physical-write reduction:

```text
12.50%
```

The test passed.

---

# 14. Invalid Parameter Testing

Invalid command-line parameters were tested to verify that the simulator fails safely.

## 14.1 Invalid Queue Count

Input:

```bash
--queues 0
```

Expected behavior:

```text
Error: --queues must be greater than 0
```

The invalid configuration is rejected.

---

## 14.2 Invalid Address Space

Input:

```bash
--address-space 0
```

Expected behavior:

```text
Error: --address-space must be greater than 0
```

The invalid configuration is rejected before storage operations begin.

---

## 14.3 Invalid Zipf Skew

Input:

```bash
--zipf-skew -1
```

Expected behavior:

```text
Error: --zipf-skew must be greater than or equal to 0
```

The invalid configuration is rejected.

---

## 14.4 Invalid Distribution

Only the following distributions are accepted:

```text
uniform
zipf
```

An invalid distribution value produces an error:

```text
Error: --distribution must be uniform or zipf
```

This prevents unknown distribution values from being silently interpreted as Zipfian workloads.

---

# 15. Application Thread Validation

The application thread configuration was also tested with zero threads.

The implementation normalizes the number of application threads to at least one:

```cpp
int nthreads = std::max(1, cfg.app_threads);
```

Therefore:

```text
--app-threads 0
```

does not result in a zero-thread execution path.

The test completed successfully.

---

# 16. Zero Zipf Skew Test

A Zipf workload with:

```text
--zipf-skew 0
```

was tested.

The simulator completed successfully.

The workload did not produce significant coalescing, which is consistent with the selected workload characteristics.

The test passed.

---

# 17. Regression Test

A representative configuration was selected as a regression workload.

```bash
./nvme_wt_sim \
  --requests 1000 \
  --address-space 100 \
  --queues 8 \
  --distribution zipf \
  --zipf-skew 1.5 \
  --app-threads 8 \
  --flush-interval-us 200 \
  --batch-trigger 64 \
  --sim-latency-us 60
```

## Baseline Result

```text
Logical Writes:      1000
Physical Writes:     1000
Average Latency:     993.93 us
p50 Latency:         850.61 us
p95 Latency:         1702.65 us
p99 Latency:         1996.48 us
Max Latency:         2478.57 us
Application IOPS:    7326.10
Application Throughput: 28.62 MB/s
```

## Optimized Result

```text
Logical Writes:      1000
Physical Writes:     596
Coalescing Reduction: 40.40%
Average Latency:     739.95 us
p50 Latency:         681.19 us
p95 Latency:         1134.34 us
p99 Latency:         1782.69 us
Max Latency:         2188.00 us
Application IOPS:    10627.39
Application Throughput: 41.51 MB/s
```

The regression test completed successfully.

---

# 18. Real Linux I/O Test

The simulator was also tested with:

```bash
--sim-latency-us 0
```

This disables the modeled service-time component and allows the simulator to use the timing of real Linux `O_DIRECT` `pwrite()` operations.

## Baseline

```text
Logical Writes:      1000
Physical Writes:     1000
Average Latency:     1166.16 us
p50 Latency:         984.20 us
p95 Latency:         2093.57 us
p99 Latency:         4223.11 us
Max Latency:         9982.51 us
Application IOPS:    6551.38
Application Throughput: 25.59 MB/s
```

## Optimized

```text
Logical Writes:      1000
Physical Writes:     644
Coalescing Reduction: 35.60%
Average Latency:     830.28 us
p50 Latency:         788.95 us
p95 Latency:         1280.46 us
p99 Latency:         1654.47 us
Max Latency:         2303.33 us
Application IOPS:    9521.65
Application Throughput: 37.19 MB/s
```

The real Linux I/O test completed successfully.

---

# 19. Representative Stage 5 Test

A larger representative workload was also executed.

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

## Baseline

```text
Logical Writes:      1000
Physical Writes:     1000
Average Latency:     1277.63 us
p50 Latency:         863.81 us
p95 Latency:         2646.16 us
p99 Latency:         3595.48 us
Max Latency:         5499.65 us
Application IOPS:    5565.31
Application Throughput: 21.74 MB/s
Average Queue Utilization: 4.17%
```

## Optimized

```text
Logical Writes:      1000
Physical Writes:     565
Coalescing Reduction: 43.50%
Average Latency:     1023.12 us
p50 Latency:         967.44 us
p95 Latency:         1695.06 us
p99 Latency:         2345.69 us
Max Latency:         2808.62 us
Application IOPS:    7690.22
Application Throughput: 30.04 MB/s
Average Queue Utilization: 3.25%
```

The comparison shows:

```text
Physical operations: 43.50% fewer
Application IOPS:    1.38x
Application throughput: 1.38x
```

The test completed successfully.

---

# 20. Reliability Observations

The testing identified several important observations.

## 20.1 Optimization Overhead

The optimized path can introduce additional overhead because it uses:

- Background flushing
- Mutex synchronization
- Condition variables
- Pending-write management
- Promise/future completion
- Batch management

Therefore, the optimized implementation is not expected to outperform the baseline for every possible workload.

---

## 20.2 Workload Locality

Coalescing effectiveness depends strongly on address locality.

Low-locality workloads may produce little physical-write reduction.

High-locality workloads can provide many opportunities for multiple logical writes to map to the same pending LBA.

---

## 20.3 Queue Count

The number of queues affects how physical write operations are distributed.

The multi-queue implementation was tested with both single-queue and multi-queue configurations.

---

## 20.4 Batch Size

The batch trigger affects when pending writes are flushed.

A small trigger can cause more frequent flushes, while a larger trigger provides more opportunity for batching and coalescing.

---

# 21. Reliability Limitations

The current prototype has several known limitations.

## 21.1 Software NVMe Model

The NVMe queue behavior is modeled in software.

It does not directly communicate with a physical NVMe controller.

---

## 21.2 Backing File

The storage backend uses a Linux backing file instead of a physical NVMe namespace.

---

## 21.3 WSL2 Environment

Testing has been performed under Ubuntu on WSL2.

Kernel-level driver loading has not been demonstrated in this environment.

---

## 21.4 Physical I/O Error Propagation

The current implementation logs a `pwrite()` failure, but the command completion path does not yet propagate a detailed physical I/O error back through the complete cache/future interface.

Therefore, physical I/O error handling remains an area for future improvement.

---

## 21.5 Constructor Error Handling

There is a minor resource-management edge case if backing-file initialization fails after the file descriptor has already been opened.

This has not affected the current test runs but remains a possible area for cleanup.

---

# 22. Final Automated Test Result

The complete automated test suite currently reports:

```text
Workload generator tests: PASS
Metrics collector tests: PASS
Cache/device integration tests: PASS
Coalescing test: PASS
Logical writes: 8
Physical writes: 1
```

The dedicated coalescing test demonstrates that multiple logical writes can be reduced to a smaller number of physical operations.

---

# 23. Reliability Test Summary

| Test Area | Result |
|---|---|
| Build validation | PASS |
| Workload generator | PASS |
| Metrics collector | PASS |
| Cache/device integration | PASS |
| Write coalescing | PASS |
| Minimum workload | PASS |
| Zero requests | PASS |
| Single-thread workload | PASS |
| Single queue | PASS |
| Multi-queue workload | PASS |
| Zipfian workload | PASS |
| High-locality workload | PASS |
| Batch boundary | PASS |
| Invalid queue count | PASS |
| Invalid address space | PASS |
| Invalid Zipf skew | PASS |
| Invalid distribution | PASS |
| Zero application threads | PASS |
| Real Linux I/O | PASS |
| Regression workload | PASS |

---

# 24. Conclusion

The Stage 5 reliability and testing activities verified the major functional paths of the Write-Through Caching NVMe Accelerator Simulator.

The testing confirmed:

- Correct workload generation
- Correct metrics collection
- Successful baseline writes
- Successful optimized writes
- Correct cache/device integration
- Working write coalescing
- Working batching
- Working multi-queue processing
- Safe handling of several invalid configurations
- Successful execution under modeled and real Linux I/O timing

The results also demonstrate that optimization effectiveness depends on workload characteristics, particularly address locality, concurrency, batching opportunities, and system parameters.

The simulator is therefore suitable for the final project demonstration and evaluation, with the documented limitations taken into account.

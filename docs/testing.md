# Testing Documentation

## 1. Purpose

This document describes the testing strategy used for the **Write-Through Caching NVMe Accelerator Simulator**.

The objective of testing is to verify:

- Correctness of the baseline write-through cache
- Correctness of the optimized write-through cache
- Write coalescing behavior
- Batching behavior
- Multi-queue processing
- Workload generation
- Metrics collection
- Edge-case handling
- Performance behavior under different workloads
- Regression stability after code changes

---

## 2. Testing Approach

Testing is divided into multiple levels:

1. **Unit Testing**
2. **Integration Testing**
3. **Coalescing Testing**
4. **Workload Testing**
5. **Performance Testing**
6. **Reliability and Edge-Case Testing**
7. **Regression Testing**
8. **Real Linux I/O Testing**

The project uses C++ test programs compiled and executed in the Linux/WSL2 environment.

---

## 3. Automated Tests

The project includes automated tests for the major software components.

### Workload Generator Test

Validates:

- Uniform workload generation
- Zipfian workload generation
- Request count
- LBA generation
- Deterministic behavior when required

Expected result:

```text
Workload generator tests: PASS
### Metrics Collector Test

Validates:

- Logical write counting
- Physical write counting
- Latency collection
- Percentile calculation
- Throughput calculation
- IOPS calculation

Expected result:

```text
Metrics collector tests: PASS
### Cache/Device Integration Test

Validates the interaction between:

- Write-through cache
- NVMe device model
- Queue processing
- Linux file-backed storage

Expected result:

```text
Cache/device integration tests: PASS
```

### Coalescing Test

Validates that multiple writes targeting the same LBA can be coalesced into fewer physical writes.

Example result:

```text
Coalescing test: PASS
Logical writes: 8
Physical writes: 1
```

This confirms that repeated writes to the same block can be reduced to a single physical write within the coalescing window.
---

## 4. Running the Automated Tests

The complete automated test suite can be executed using:

```bash
make test
```

The tests should complete successfully before considering a code change ready for further benchmarking.

---

## 5. Functional Testing

Functional testing verifies that the simulator performs the required operations correctly.

The following functionality is tested:

- Workload generation
- Cache insertion
- Write-through behavior
- Physical write submission
- Queue selection
- Background flushing
- Write coalescing
- Batch processing
- Metrics collection
- Command-line argument parsing

---

## 6. Baseline Cache Testing

The baseline cache is tested using:

- Single-threaded workloads
- Multi-threaded workloads
- Uniform workloads
- Zipfian workloads
- Different address-space sizes
- Different queue counts

The baseline implementation is expected to perform a physical write for every logical write.

For example:

```text
Logical writes: 1000
Physical writes: 1000
```

This provides the reference behavior against which the optimized implementation can be compared.

---

## 7. Optimized Cache Testing

The optimized cache is tested for:

- Write coalescing
- Batching
- Background flushing
- Multi-queue dispatch
- Queue balancing
- Correct completion of waiting requests

For workloads with repeated writes to the same LBA, the optimized implementation should reduce the number of physical writes.

Example:

```text
Logical writes: 1000
Physical writes: 596
Coalescing reduction: 40.40%
```

---

## 8. Workload Testing

Two workload distributions are supported:

### Uniform Distribution

Each LBA has approximately equal probability of being selected.

Example:

```bash
./nvme_wt_sim \
  --requests 1000 \
  --address-space 100 \
  --distribution uniform \
  --app-threads 8
```

Uniform workloads are useful for evaluating general behavior with relatively low locality.

### Zipfian Distribution

Some LBAs are accessed more frequently than others.

Example:

```bash
./nvme_wt_sim \
  --requests 1000 \
  --address-space 100 \
  --distribution zipf \
  --zipf-skew 1.5 \
  --app-threads 8
```

Zipfian workloads are useful for testing the effectiveness of write coalescing.

---

## 9. Multi-Thread Testing

The simulator supports multiple application threads.

Example:

```bash
./nvme_wt_sim \
  --requests 1000 \
  --address-space 100 \
  --queues 8 \
  --distribution zipf \
  --zipf-skew 1.5 \
  --app-threads 8 \
  --sim-latency-us 60
```

Multi-thread testing verifies:

- Thread synchronization
- Shared cache access
- Concurrent request generation
- Queue processing
- Background flushing
- Completion handling

---

## 10. Multi-Queue Testing

The optimized implementation distributes physical writes across multiple simulated NVMe queue pairs.

Testing is performed with different queue counts, including:

```text
1 queue
4 queues
8 queues
```

The purpose is to verify:

- Correct queue selection
- Concurrent queue processing
- Queue balancing
- Absence of deadlocks
- Stable completion behavior

Queue utilization is recorded for each queue.

---

## 11. Write Coalescing Testing

Write coalescing is one of the main optimization mechanisms in the project.

The test generates multiple writes to the same logical block.

Example:

```text
Logical writes: 8
Physical writes: 1
```

The expected behavior is:

```text
Multiple logical writes
        ↓
Same LBA
        ↓
Pending writes merged
        ↓
One physical write
```

This demonstrates the reduction in physical I/O operations.

---

## 12. Batching Testing

The optimized cache uses a background flush mechanism.

Pending writes are collected into a batch before being submitted to the simulated NVMe device.

The following parameters can be varied:

```text
--flush-interval-us
--batch-trigger
```

Example:

```bash
./nvme_wt_sim \
  --requests 1000 \
  --address-space 100 \
  --distribution zipf \
  --zipf-skew 1.5 \
  --app-threads 8 \
  --flush-interval-us 200 \
  --batch-trigger 64 \
  --sim-latency-us 60
```

Testing verifies that pending requests are eventually flushed even when the batch-trigger threshold is not reached.

## 13. Edge-Case Testing

The simulator is tested with boundary and invalid input values.

Examples include:

- Zero requests
- One request
- One application thread
- One queue
- Multiple queues
- Very small address space
- Large address space
- Zero Zipf skew
- Invalid Zipf skew
- Invalid queue count
- Invalid address-space size
- Invalid distribution value

The program should reject invalid arguments cleanly instead of continuing with invalid configuration.

---

## 14. Zero-Request Test

The simulator is tested with:

```text
--requests 0
```

Expected behavior:

- No crash
- No physical writes
- No logical writes
- Valid zero-valued metrics

Example:

```text
Logical writes: 0
Physical writes: 0
```

---

## 15. Single-LBA Stress Test

A deliberately constructed workload is used where all requests target a very small address space.

Example:

```bash
./nvme_wt_sim \
  --requests 1000 \
  --address-space 1 \
  --distribution uniform \
  --app-threads 8 \
  --queues 8 \
  --sim-latency-us 60
```

This creates a strong opportunity for write coalescing.

The test demonstrates that the optimized cache can substantially reduce physical writes when many logical writes target the same block.

---

## 16. Regression Testing

A representative workload is repeatedly executed after code changes.

Example:

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

The purpose of regression testing is to ensure that changes to:

- argument validation
- cache implementation
- queue handling
- workload generation
- metrics
- documentation-related source changes

do not introduce functional regressions.

---

## 17. Representative Regression Results

A representative regression run produced the following results.

### Baseline

```text
Logical writes:       1000
Physical writes:      1000
Average latency:      993.93 us
P50 latency:          850.61 us
P95 latency:          1702.65 us
P99 latency:          1996.48 us
Application IOPS:     7326.10
Application throughput: 28.62 MB/s
```

### Optimized

```text
Logical writes:       1000
Physical writes:      596
Coalescing reduction: 40.40%
Average latency:      739.95 us
P50 latency:          681.19 us
P95 latency:          1134.34 us
P99 latency:          1782.69 us
Application IOPS:     10627.39
Application throughput: 41.51 MB/s
```

The run demonstrates that the optimized implementation can reduce physical operations while maintaining correct logical write completion.

---

## 18. Real Linux I/O Testing

The simulator can also be executed using real Linux `O_DIRECT` file I/O by setting:

```text
--sim-latency-us 0
```

Example:

```bash
./nvme_wt_sim \
  --requests 1000 \
  --address-space 1000 \
  --queues 8 \
  --distribution zipf \
  --zipf-skew 1.5 \
  --app-threads 8 \
  --sim-latency-us 0
```

This mode uses Linux file I/O rather than an artificially modeled device service delay.

The purpose is to observe simulator behavior with actual Linux storage-system calls.

---

## 19. Simulated Device Timing

The simulator also supports modeled device latency.

For example:

```text
--sim-latency-us 60
```

adds an approximately modeled device service delay.

This allows repeatable experiments without depending entirely on the performance characteristics of the host storage device.

Both modes are useful for understanding the difference between:

- Modeled device behavior
- Actual Linux file-backed I/O

---

## 20. Metrics Verified During Testing

The simulator records several performance metrics.

### Latency

- Average latency
- P50 latency
- P95 latency
- P99 latency
- Maximum latency

### Throughput

- Application throughput
- Physical write throughput

### IOPS

- Application IOPS
- Physical IOPS

### Write Reduction

```text
Coalescing Reduction =
(Logical Writes - Physical Writes)
/
Logical Writes × 100
```

### Queue Utilization

Queue utilization is reported for individual queues and as an average across queues.

---

## 21. Reliability Observations

Testing demonstrated that the simulator:

- Handles zero-request workloads
- Handles single-request workloads
- Handles single-threaded execution
- Handles multi-threaded execution
- Handles multiple simulated queues
- Handles uniform workloads
- Handles Zipfian workloads
- Performs write coalescing
- Performs background batching
- Rejects invalid configuration values
- Completes automated tests successfully
- Supports both modeled and real Linux I/O modes

---

## 22. Known Reliability Limitations

The current project is a learning-oriented simulator rather than a production storage system.

One known limitation is related to physical I/O errors.

If a Linux `pwrite()` operation fails, the current implementation reports the error, but the completion path does not propagate a full failure state through the cache request lifecycle.

Therefore, the simulator currently does not implement a complete production-grade I/O error recovery mechanism.

This limitation is documented rather than addressed with a larger error-handling framework because the project focuses on demonstrating:

- C++ system programming
- Linux I/O
- caching
- batching
- coalescing
- multi-queue processing

---

## 23. Test Execution Environment

Testing was performed in a Linux environment using Ubuntu on WSL2.

The project uses:

```text
C++17
GNU g++
Make
Linux
O_DIRECT
pwrite()
posix_memalign()
std::thread
std::mutex
std::condition_variable
std::future
std::promise
C++ STL
```

---

## 24. Final Test Result

The final automated test suite completed successfully:

```text
Workload generator tests: PASS
Metrics collector tests: PASS
Cache/device integration tests: PASS
Coalescing test: PASS
```

The project was also tested using:

- Boundary workloads
- Invalid parameters
- Uniform distributions
- Zipfian distributions
- Single-LBA workloads
- Multi-threaded workloads
- Multi-queue workloads
- Modeled device latency
- Real Linux `O_DIRECT` I/O

---

## 25. Conclusion

The testing process verifies the major functional components of the Write-Through Caching NVMe Accelerator Simulator.

The results confirm that the implementation can:

1. Generate different storage workloads.
2. Perform baseline write-through processing.
3. Perform optimized write-through processing.
4. Coalesce repeated writes.
5. Batch pending operations.
6. Distribute writes across multiple simulated queues.
7. Collect latency and throughput metrics.
8. Handle invalid and boundary inputs.
9. Execute using Linux file-backed I/O.
10. Pass the project's automated test suite.

The testing results provide the required evidence that the simulator is functionally working and suitable for demonstration and evaluation as a Linux/C++ systems programming project.

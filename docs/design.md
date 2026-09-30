# Detailed Data Flow

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

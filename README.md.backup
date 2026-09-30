# Write-Through Caching NVMe Accelerator Simulator

A C++17 / Linux simulator that implements a conventional (naive) write-through
cache and an optimized write-through cache over the same modeled NVMe device,
and measures the difference in latency, throughput, IOPS, queue utilization,
and physical storage operations.

## 1. Problem being addressed

Write-through caching gives strong durability (every write is propagated to
storage before it is acknowledged) at the cost of latency, because the
application waits on storage for every write. Modern NVMe SSDs expose many
parallel submission/completion queues, but a naive software write path that
submits one command at a time on one queue and blocks for its completion
never uses that parallelism. This project builds both implementations side
by side, over the same device model, and quantifies the gap.

## 2. How the two caches differ

| | Baseline (`BaselineWriteThroughCache`) | Optimized (`OptimizedWriteThroughCache`) |
|---|---|---|
| Queue usage | Always queue 0, queue depth 1 | Round-robin across all queue pairs |
| Submission | One `pwrite()`-equivalent command per request, blocking | Background thread batches pending writes and dispatches the whole batch at once |
| Duplicate writes | Every write is a separate physical op | Writes to the same block inside a flush window are coalesced into one physical op |
| Durability | Ack after that command completes | Ack after the (possibly coalesced) command completes — still write-through, never acks before the data is on the device |

The correctness argument for coalescing: if block X is written twice before
either reaches the device, only the newer value is ever meaningful — writing
the older value first would just be overwritten before anyone could observe
it. So folding both logical writes into a single physical write of the final
value, and acknowledging both callers when that one write completes, gives
the same durability guarantee (final value is on stable storage before
being acknowledged) with fewer physical operations. This is standard
last-writer-wins coalescing, not a weakening of write-through semantics.

## 3. Where the four topics show up

**Linux.** The device model issues real `pwrite()` syscalls against a file
opened with `O_DIRECT` (bypassing the page cache, so writes actually go
through the block I/O path instead of being absorbed by DRAM), using
`posix_memalign` for the DMA-alignment `O_DIRECT` requires. Concurrency is
built on POSIX threads via `std::thread`, `std::mutex`, and
`std::condition_variable` (the "doorbell" a real NVMe driver rings to tell
the device new commands are pending is modeled directly with a
`condition_variable::notify_one()`).

**C++.** RAII (`AlignedBuffer` frees its aligned buffer in its destructor,
the device closes and unlinks its backing file in its own destructor), STL
containers (`std::unordered_map` for both the in-memory cache and the
pending-coalescing table, `std::deque` for the submission queue), smart
pointers (`std::shared_ptr<AlignedBuffer>`), `std::atomic` counters for
lock-free statistics, `std::future`/`std::promise` to signal write
completion back to the calling thread, move semantics, and separated
headers/implementation across a small multi-file project built with a
Makefile.

**Computer architecture.** The `NVMeQueuePair` objects model an SSD's
internal channel parallelism — multiple independent servers rather than one.
Queue depth and outstanding-command counts are tracked directly
(`max_depth_seen`). The measured relationship between concurrency, latency,
and throughput is Little's Law in practice: pushing more requests
concurrently through the optimized path raises per-request latency somewhat
(more queueing) while raising aggregate throughput a lot — the classic
batching latency/throughput trade-off, visible directly in the numbers this
simulator prints. Write-through vs. write-back is a cache-consistency model
choice, and the coalescing design is explicitly justified as memory/storage
consistency reasoning, not just an engineering trick.

**Hardware and software.** An NVMe queue pair *is* a submission ring the
driver pushes commands into and a completion ring the device pushes
results into; that's exactly what `NVMeQueuePair` implements, with a
dedicated worker thread standing in for the device-side execution unit for
each channel. The project also makes the real-hardware caveat explicit: see
Section 5.

## 4. Build and run

```
make            # builds ./nvme_wt_sim
./nvme_wt_sim --help
```

Example run (skewed/Zipfian workload, modeled 60us channel latency, 8 queues,
64 concurrent application threads):

```
./nvme_wt_sim --requests 20000 --address-space 50000 --queues 8 \
  --distribution zipf --zipf-skew 1.2 --app-threads 64 \
  --flush-interval-us 40 --batch-trigger 32 --sim-latency-us 60
```

This prints a full report for each cache and a comparison table, and writes
`results/comparison.csv`. To turn that into charts:

```
python3 scripts/plot_results.py results/comparison.csv results/chart.png
```

Key flags:
- `--distribution uniform|zipf` and `--zipf-skew` control address skew —
  coalescing only helps when addresses repeat, so Zipfian (hot-block) traffic
  is where its benefit shows up; uniform traffic isolates the multi-queue
  parallelism benefit instead.
- `--app-threads` sets how many concurrent "application" threads submit
  writes — both caches are always run under the *same* concurrency, because
  comparing a single-threaded baseline against a multi-threaded optimized
  run would be an unfair comparison.
- `--sim-latency-us` (default 60) uses an analytical per-op channel-latency
  model instead of this host's raw `pwrite()` timing; see Section 5 for why.
  Pass `0` to use real O_DIRECT timing end to end.

## 5. An honest note on real vs. modeled device timing

Everything in this project performs real `pwrite()` calls to a real file
opened `O_DIRECT`; that part is not simulated. What *is* optionally modeled
is the per-operation service time used for the latency/throughput numbers.

On the sandbox this was developed in (a single-vCPU VM over a shared/
virtualized disk), real, unmodified `O_DIRECT` timing shows only a marginal
multi-queue benefit on a uniform-address workload —
`results/run_uniform_real_odirect.log` (`--sim-latency-us 0`) measures
roughly **1.03x** on latency/IOPS, essentially parity, because the host's
storage stack and single vCPU don't give the eight "queues" genuine
independent hardware channels to exploit at any real scale; almost all of
the improvement that *does* show up in real timing comes from coalescing,
and only when the workload has address skew
(`results/run_zipf_real_odirect.log`).

Rather than hide that, `--sim-latency-us` (on by default, 60us base ±15%
jitter per queue) replaces the *measured* per-op time with an *analytical*
one representative of real multi-channel NVMe hardware, while still issuing
the real syscall for correctness. This is the standard reason architecture
simulators use service-time models instead of raw measurements from an
unrepresentative host: it isolates the software architecture under test
(queueing, batching, coalescing) from a specific machine's incidental
limitations. Both modes are in `results/` as evidence:

- `run_zipf_modeled.log` / `chart_zipf_modeled.png` — modeled latency,
  skewed workload: coalescing + parallelism combined (~5x latency and IOPS
  improvement).
- `run_uniform_modeled.log` / `chart_uniform_modeled.png` — modeled latency,
  uniform workload: parallelism benefit in isolation (~2.7x improvement,
  negligible coalescing as expected).
- `run_zipf_real_odirect.log` — unmodified real O_DIRECT timing, skewed
  workload, on this host: still shows the coalescing win (fewer physical
  ops, higher latency/IOPS from that alone) even without a parallelism win.

## 6. Project layout

```
include/    nvme_device.hpp, metrics.hpp, workload.hpp, write_through_cache.hpp
src/        matching .cpp files + main.cpp (the driver/CLI)
scripts/    plot_results.py (optional chart generation from the CSV output)
results/    saved run logs, CSVs, and charts from three representative runs
Makefile
```

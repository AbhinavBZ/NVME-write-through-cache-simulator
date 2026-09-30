// write_through_cache.hpp
//
// Two write-through caches over the same NVMeDevice model:
//
//   BaselineWriteThroughCache
//     Conventional/naive implementation. Every write: update the
//     in-memory cache line, then synchronously submit ONE command to
//     ONE queue and BLOCK until it completes before accepting the next
//     write. Queue depth is effectively 1 and only one of the device's
//     parallel channels is ever used -- this is exactly the software
//     bottleneck described in the problem statement: the NVMe device's
//     parallelism goes unused because the host never issues more than
//     one outstanding command.
//
//   OptimizedWriteThroughCache
//     Write-through consistency is still preserved (a request is only
//     acknowledged once its data, or a later overwrite of it, is
//     confirmed durable on the device) but the write path is
//     accelerated by:
//       1. Write coalescing: multiple writes to the same LBA inside a
//          short flush window collapse into a single physical write of
//          the latest value. All folded requests are acknowledged
//          together when that one physical write completes -- correct,
//          because any earlier value was superseded before it was ever
//          made durable.
//       2. Batching: a background flush thread periodically drains the
//          pending-write table and dispatches the whole batch at once,
//          instead of one syscall-and-wait per request.
//       3. Multi-queue, deep-pipeline dispatch: the batch is spread
//          round-robin across every queue pair the device exposes, so
//          many commands are outstanding concurrently (queue depth >> 1
//          and every channel is used), which is what lets the host
//          approach the device's real achievable IOPS/throughput.
#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "metrics.hpp"
#include "nvme_device.hpp"

namespace nvmesim {

// ---------------------------------------------------------------------
class BaselineWriteThroughCache {
public:
    BaselineWriteThroughCache(NVMeDevice& device, MetricsCollector& metrics);

    // Blocks the calling "application thread" until the write is durable,
    // exactly as a naive write-through cache would.
    void write(uint64_t lba, const std::vector<char>& value);

private:
    NVMeDevice& device_;
    MetricsCollector& metrics_;
    std::mutex cache_mtx_;
    std::unordered_map<uint64_t, std::vector<char>> cache_; // in-memory write-through cache
};

// ---------------------------------------------------------------------
class OptimizedWriteThroughCache {
public:
    OptimizedWriteThroughCache(NVMeDevice& device, MetricsCollector& metrics,
                                std::chrono::microseconds flush_interval,
                                size_t batch_size_trigger);
    ~OptimizedWriteThroughCache();

    // Non-blocking from the caller's perspective in the sense that it
    // doesn't issue its own syscall; it still only returns once the
    // write (or whatever superseded it) is durable, preserving
    // write-through semantics.
    void write(uint64_t lba, const std::vector<char>& value);

    void start();
    void stop();

private:
    struct PendingEntry {
        std::vector<char> data;
        std::vector<std::shared_ptr<std::promise<void>>> waiters;
    };

    void flush_loop();
    void flush_now();

    NVMeDevice& device_;
    MetricsCollector& metrics_;
    std::chrono::microseconds flush_interval_;
    size_t batch_size_trigger_;

    std::mutex cache_mtx_;
    std::unordered_map<uint64_t, std::vector<char>> cache_; // in-memory write-through cache

    std::mutex pending_mtx_;
    std::unordered_map<uint64_t, PendingEntry> pending_;

    std::atomic<bool> running_{false};
    std::thread flusher_;
    std::condition_variable flush_cv_;
    std::mutex flush_cv_mtx_;
    std::atomic<bool> flush_now_flag_{false};
};

} // namespace nvmesim

// nvme_device.hpp
//
// Software model of an NVMe SSD's submission/completion queue architecture.
//
// Concepts demonstrated here (mapped explicitly for the project write-up):
//   - Linux:  O_DIRECT unbuffered I/O (bypasses the page cache so writes really
//             go through the block layer instead of being absorbed by DRAM
//             cache), posix_memalign for DMA-alignment, pwrite()/fsync(),
//             pthread-backed std::thread, mutex/condition_variable.
//   - C++:    RAII (AlignedBuffer, DeviceFile), STL containers, std::atomic,
//             std::thread, move semantics, smart pointers.
//   - Comp. Arch: multiple independent queue pairs model an SSD's internal
//             channel parallelism; "doorbell" (condition_variable notify)
//             and "completion" mirror real NVMe SQ/CQ semantics; queue
//             depth / outstanding-command modeling; Little's Law shows up
//             directly in the utilization numbers this class reports.
//   - HW/SW:  an NVMe queue pair is exactly this: a ring the driver pushes
//             commands into (SQ) and a ring the device pushes completions
//             into (CQ). We approximate the ring with a mutex-protected
//             deque and the doorbell with a condition_variable, and we
//             approximate the device-side execution with a dedicated
//             worker thread issuing a real pwrite() syscall.

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <fcntl.h>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace nvmesim {

using Clock = std::chrono::steady_clock;

// ---------------------------------------------------------------------
// AlignedBuffer: RAII wrapper around a posix_memalign'd buffer.
// O_DIRECT requires the user buffer, the file offset, and the transfer
// length to all be aligned to the device's logical block size (usually
// 512B, we use 4096B to be safe on modern drives).
// ---------------------------------------------------------------------
class AlignedBuffer {
public:
    explicit AlignedBuffer(size_t size, size_t alignment = 4096);
    ~AlignedBuffer();
    AlignedBuffer(const AlignedBuffer&) = delete;
    AlignedBuffer& operator=(const AlignedBuffer&) = delete;
    AlignedBuffer(AlignedBuffer&& other) noexcept;

    char* data() const { return ptr_; }
    size_t size() const { return size_; }

private:
    char* ptr_ = nullptr;
    size_t size_ = 0;
};

// ---------------------------------------------------------------------
// A single write command submitted to a queue pair.
// ---------------------------------------------------------------------
struct WriteCommand {
    uint64_t lba;                 // logical block address (block index)
    uint32_t num_blocks;          // number of contiguous blocks
    std::shared_ptr<AlignedBuffer> data;
    Clock::time_point enqueue_time;      // when it entered the SQ
    std::promise<Clock::time_point> completion; // fulfilled at CQ time
    uint64_t merged_request_count = 1;   // >1 if this command represents
                                          // several coalesced logical writes
};

struct QueueStats {
    std::atomic<uint64_t> ops_completed{0};
    std::atomic<uint64_t> bytes_written{0};
    std::atomic<uint64_t> busy_ns{0};      // time spent actually servicing pwrite()
    std::atomic<uint64_t> max_depth_seen{0};
};

// ---------------------------------------------------------------------
// NVMeQueuePair: one submission queue (SQ) + completion queue (CQ),
// serviced by a dedicated worker thread -- this is the unit of hardware
// parallelism an NVMe device exposes (real drives commonly expose
// dozens to thousands of these).
// ---------------------------------------------------------------------
class NVMeQueuePair {
public:
    // sim_latency_ns == 0  -> "real" mode: the completion latency reported
    //                         is whatever the real O_DIRECT pwrite() call
    //                         actually took on this host.
    // sim_latency_ns  > 0  -> "modeled" mode: pwrite() is still issued for
    //                         genuine durability/correctness, but the
    //                         reported per-op service time is drawn from
    //                         an analytical channel-latency model
    //                         (base +/-15% jitter) instead of the real
    //                         syscall duration. This isolates the
    //                         queueing/parallelism architecture under
    //                         test from this host's particular storage
    //                         stack (virtualized disk, single vCPU
    //                         scheduling noise, etc.), which is the
    //                         standard reason architecture simulators use
    //                         analytical service-time models rather than
    //                         raw measurements from an unrepresentative
    //                         host.
    NVMeQueuePair(int fd, size_t block_size, int queue_id, uint64_t sim_latency_ns = 0);
    ~NVMeQueuePair();

    void start();
    void stop();

    // "Ring the doorbell": push a command into the SQ and wake the worker.
    std::future<Clock::time_point> submit(WriteCommand cmd);

    size_t depth() const;
    const QueueStats& stats() const { return stats_; }

private:
    void worker_loop();

    int fd_;
    size_t block_size_;
    int queue_id_;
    uint64_t sim_latency_ns_;
    std::mt19937 jitter_rng_;

    std::deque<WriteCommand> sq_;
    mutable std::mutex mtx_;
    std::condition_variable doorbell_;
    std::atomic<bool> running_{false};
    std::thread worker_;
    QueueStats stats_;
};

// ---------------------------------------------------------------------
// NVMeDevice: owns the backing file (the "physical media") and a set of
// parallel queue pairs. Dispatch policy is round-robin across queues,
// mirroring how a driver spreads I/O across a multi-queue device.
// ---------------------------------------------------------------------
class NVMeDevice {
public:
    NVMeDevice(const std::string& backing_path, uint64_t num_blocks,
               size_t block_size, int num_queues, uint64_t sim_latency_ns = 0);
    ~NVMeDevice();

    // Submit one write, dispatched to queue (round-robin unless queue_hint >= 0).
    std::future<Clock::time_point> submit_write(uint64_t lba, uint32_t num_blocks,
                                                 std::shared_ptr<AlignedBuffer> data,
                                                 int queue_hint = -1);

    int num_queues() const { return (int)queues_.size(); }
    size_t block_size() const { return block_size_; }
    NVMeQueuePair& queue(int i) { return *queues_[i]; }

    // Aggregate stats across all queues.
    uint64_t total_physical_ops() const;
    uint64_t total_bytes_written() const;
    uint64_t total_busy_ns() const;
    std::vector<double> per_queue_utilization(double wall_seconds) const;

private:
    std::string backing_path_;
    int fd_ = -1;
    size_t block_size_;
    uint64_t num_blocks_;
    std::vector<std::unique_ptr<NVMeQueuePair>> queues_;
    std::atomic<uint64_t> rr_counter_{0};
};

} // namespace nvmesim

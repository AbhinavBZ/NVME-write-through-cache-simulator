#include "nvme_device.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

namespace nvmesim {

// ---------------------------- AlignedBuffer ----------------------------

AlignedBuffer::AlignedBuffer(size_t size, size_t alignment) : size_(size) {
    void* p = nullptr;
    if (posix_memalign(&p, alignment, size) != 0) {
        throw std::runtime_error("posix_memalign failed");
    }
    ptr_ = static_cast<char*>(p);
    std::memset(ptr_, 0, size_);
}

AlignedBuffer::~AlignedBuffer() {
    if (ptr_) free(ptr_);
}

AlignedBuffer::AlignedBuffer(AlignedBuffer&& other) noexcept
    : ptr_(other.ptr_), size_(other.size_) {
    other.ptr_ = nullptr;
    other.size_ = 0;
}

// ---------------------------- NVMeQueuePair ----------------------------

NVMeQueuePair::NVMeQueuePair(int fd, size_t block_size, int queue_id, uint64_t sim_latency_ns)
    : fd_(fd), block_size_(block_size), queue_id_(queue_id),
      sim_latency_ns_(sim_latency_ns), jitter_rng_(1000 + queue_id) {}

NVMeQueuePair::~NVMeQueuePair() { stop(); }

void NVMeQueuePair::start() {
    running_ = true;
    worker_ = std::thread(&NVMeQueuePair::worker_loop, this);
}

void NVMeQueuePair::stop() {
    if (!running_) return;
    running_ = false;
    doorbell_.notify_all();
    if (worker_.joinable()) worker_.join();
}

size_t NVMeQueuePair::depth() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return sq_.size();
}

std::future<Clock::time_point> NVMeQueuePair::submit(WriteCommand cmd) {
    auto fut = cmd.completion.get_future();
    {
        std::lock_guard<std::mutex> lk(mtx_);
        sq_.push_back(std::move(cmd));
        uint64_t d = sq_.size();
        uint64_t prev = stats_.max_depth_seen.load();
        while (d > prev && !stats_.max_depth_seen.compare_exchange_weak(prev, d)) {}
    }
    doorbell_.notify_one();  // ring the doorbell
    return fut;
}

void NVMeQueuePair::worker_loop() {
    while (true) {
        WriteCommand cmd;
        {
            std::unique_lock<std::mutex> lk(mtx_);
            doorbell_.wait(lk, [this] { return !sq_.empty() || !running_; });
            if (!running_ && sq_.empty()) return;
            cmd = std::move(sq_.front());
            sq_.pop_front();
        }

        // --- Device-side execution: a real O_DIRECT pwrite() syscall. ---
        off_t offset = static_cast<off_t>(cmd.lba) * block_size_;
        size_t len = static_cast<size_t>(cmd.num_blocks) * block_size_;

        auto t0 = Clock::now();
        ssize_t n = pwrite(fd_, cmd.data->data(), len, offset);
        auto t1 = Clock::now();

        if (n < 0 || static_cast<size_t>(n) != len) {
            std::cerr << "[queue " << queue_id_ << "] pwrite failed: "
                      << strerror(errno) << "\n";
        }

        uint64_t real_busy = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        uint64_t busy = real_busy;

        if (sim_latency_ns_ > 0) {
            // Modeled channel latency: base +/- 15% jitter, independent
            // per queue pair (models per-channel variance the way a
            // real device's flash channels differ slightly).
            std::uniform_real_distribution<double> jitter(0.85, 1.15);
            uint64_t modeled = static_cast<uint64_t>(sim_latency_ns_ * jitter(jitter_rng_));
            if (modeled > real_busy) {
                std::this_thread::sleep_for(std::chrono::nanoseconds(modeled - real_busy));
            }
            busy = modeled;
            t1 = t0 + std::chrono::nanoseconds(modeled);
        }

        stats_.busy_ns += busy;
        stats_.ops_completed += cmd.merged_request_count; // logical writes serviced
        stats_.bytes_written += len;

        cmd.completion.set_value(t1);
    }
}

// ---------------------------- NVMeDevice ----------------------------

NVMeDevice::NVMeDevice(const std::string& backing_path, uint64_t num_blocks,
                        size_t block_size, int num_queues, uint64_t sim_latency_ns)
    : backing_path_(backing_path), block_size_(block_size), num_blocks_(num_blocks) {
    fd_ = open(backing_path.c_str(), O_CREAT | O_RDWR | O_DIRECT | O_TRUNC, 0644);
    if (fd_ < 0) {
        throw std::runtime_error("failed to open backing file with O_DIRECT: " +
                                  std::string(strerror(errno)));
    }
    if (ftruncate(fd_, static_cast<off_t>(num_blocks_) * block_size_) != 0) {
        throw std::runtime_error("ftruncate failed: " + std::string(strerror(errno)));
    }

    queues_.reserve(num_queues);
    for (int i = 0; i < num_queues; ++i) {
        queues_.push_back(std::make_unique<NVMeQueuePair>(fd_, block_size_, i, sim_latency_ns));
        queues_.back()->start();
    }
}

NVMeDevice::~NVMeDevice() {
    for (auto& q : queues_) q->stop();
    if (fd_ >= 0) {
        close(fd_);
        unlink(backing_path_.c_str());
    }
}

std::future<Clock::time_point> NVMeDevice::submit_write(uint64_t lba, uint32_t num_blocks,
                                                          std::shared_ptr<AlignedBuffer> data,
                                                          int queue_hint) {
    WriteCommand cmd;
    cmd.lba = lba;
    cmd.num_blocks = num_blocks;
    cmd.data = std::move(data);
    cmd.enqueue_time = Clock::now();

    int qidx = queue_hint;
    if (qidx < 0 || qidx >= (int)queues_.size()) {
        qidx = static_cast<int>(rr_counter_.fetch_add(1) % queues_.size());
    }
    return queues_[qidx]->submit(std::move(cmd));
}

uint64_t NVMeDevice::total_physical_ops() const {
    uint64_t t = 0;
    for (auto& q : queues_) t += q->stats().ops_completed.load();
    return t;
}

uint64_t NVMeDevice::total_bytes_written() const {
    uint64_t t = 0;
    for (auto& q : queues_) t += q->stats().bytes_written.load();
    return t;
}

uint64_t NVMeDevice::total_busy_ns() const {
    uint64_t t = 0;
    for (auto& q : queues_) t += q->stats().busy_ns.load();
    return t;
}

std::vector<double> NVMeDevice::per_queue_utilization(double wall_seconds) const {
    std::vector<double> util;
    for (auto& q : queues_) {
        double busy_s = q->stats().busy_ns.load() / 1e9;
        util.push_back(wall_seconds > 0 ? (busy_s / wall_seconds) * 100.0 : 0.0);
    }
    return util;
}

} // namespace nvmesim

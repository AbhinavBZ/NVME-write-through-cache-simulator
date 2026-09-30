#include "write_through_cache.hpp"

#include <cstring>

namespace nvmesim {

static double us_between(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::micro>(b - a).count();
}

// ======================= BaselineWriteThroughCache =======================

BaselineWriteThroughCache::BaselineWriteThroughCache(NVMeDevice& device, MetricsCollector& metrics)
    : device_(device), metrics_(metrics) {}

void BaselineWriteThroughCache::write(uint64_t lba, const std::vector<char>& value) {
    {
        std::lock_guard<std::mutex> lk(cache_mtx_);
        cache_[lba] = value; // update in-memory write-through cache line
    }

    auto buf = std::make_shared<AlignedBuffer>(device_.block_size());
    std::memcpy(buf->data(), value.data(), std::min(value.size(), buf->size()));

    auto t0 = Clock::now();
    // Naive path: always the SAME queue (queue 0), one outstanding
    // command at a time -- the device's other queues sit idle and no
    // pipelining occurs because we block here until completion.
    auto fut = device_.submit_write(lba, 1, buf, /*queue_hint=*/0);
    auto t_complete = fut.get();

    metrics_.record_latency_us(us_between(t0, t_complete));
}

// ======================= OptimizedWriteThroughCache =======================

OptimizedWriteThroughCache::OptimizedWriteThroughCache(NVMeDevice& device, MetricsCollector& metrics,
                                                         std::chrono::microseconds flush_interval,
                                                         size_t batch_size_trigger)
    : device_(device), metrics_(metrics), flush_interval_(flush_interval),
      batch_size_trigger_(batch_size_trigger) {}

OptimizedWriteThroughCache::~OptimizedWriteThroughCache() { stop(); }

void OptimizedWriteThroughCache::start() {
    running_ = true;
    flusher_ = std::thread(&OptimizedWriteThroughCache::flush_loop, this);
}

void OptimizedWriteThroughCache::stop() {
    if (!running_) return;
    running_ = false;
    flush_cv_.notify_all();
    if (flusher_.joinable()) flusher_.join();
    flush_now(); // drain anything left pending
}

void OptimizedWriteThroughCache::write(uint64_t lba, const std::vector<char>& value) {
    {
        std::lock_guard<std::mutex> lk(cache_mtx_);
        cache_[lba] = value; // in-memory cache is updated immediately (read-your-writes)
    }

    auto waiter = std::make_shared<std::promise<void>>();
    auto fut = waiter->get_future();
    auto submit_time = Clock::now();

    bool trigger_flush = false;
    {
        std::lock_guard<std::mutex> lk(pending_mtx_);
        auto& entry = pending_[lba];
        entry.data = value; // coalesce: last writer before flush wins
        entry.waiters.push_back(waiter);
        if (pending_.size() >= batch_size_trigger_) trigger_flush = true;
    }
    if (trigger_flush) {
        flush_now_flag_ = true;
        flush_cv_.notify_one();
    }

    fut.wait(); // block the calling "application thread" until durable
    metrics_.record_latency_us(us_between(submit_time, Clock::now()));
}

void OptimizedWriteThroughCache::flush_loop() {
    std::unique_lock<std::mutex> lk(flush_cv_mtx_);
    while (running_) {
        flush_cv_.wait_for(lk, flush_interval_,
                            [this] { return flush_now_flag_.load() || !running_; });
        flush_now_flag_ = false;
        lk.unlock();
        flush_now();
        lk.lock();
    }
}

void OptimizedWriteThroughCache::flush_now() {
    std::unordered_map<uint64_t, PendingEntry> batch;
    {
        std::lock_guard<std::mutex> lk(pending_mtx_);
        if (pending_.empty()) return;
        std::swap(batch, pending_);
    }

    // Dispatch the whole coalesced batch across every queue the device
    // exposes (round-robin), so many physical writes are outstanding at
    // once -- this is where multi-queue hardware parallelism gets used.
    std::vector<std::pair<uint64_t, std::future<Clock::time_point>>> futures;
    futures.reserve(batch.size());
    for (auto& [lba, entry] : batch) {
        auto buf = std::make_shared<AlignedBuffer>(device_.block_size());
        std::memcpy(buf->data(), entry.data.data(), std::min(entry.data.size(), buf->size()));
        auto fut = device_.submit_write(lba, 1, buf, /*queue_hint=*/-1); // round-robin
        futures.emplace_back(lba, std::move(fut));
    }

    for (auto& [lba, fut] : futures) {
        fut.get(); // completion timestamp available if finer-grained timing is needed
        auto& entry = batch[lba];
        for (auto& w : entry.waiters) w->set_value(); // ack every coalesced logical write
    }
}

} // namespace nvmesim

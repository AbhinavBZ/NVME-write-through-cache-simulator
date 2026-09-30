// metrics.hpp
// Collects per-request latency samples and derives the standard
// storage-performance figures: average/percentile latency, IOPS,
// throughput, queue utilization, and the logical-vs-physical op count
// (the direct evidence of how much coalescing saved).
#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace nvmesim {

class MetricsCollector {
public:
    void record_latency_us(double us);
    void set_logical_requests(uint64_t n) { logical_requests_ = n; }
    void set_physical_ops(uint64_t n) { physical_ops_ = n; }
    void set_bytes_written(uint64_t n) { bytes_written_ = n; }
    void set_logical_bytes_requested(uint64_t n) { logical_bytes_ = n; }
    void set_wall_seconds(double s) { wall_seconds_ = s; }
    void set_queue_utilization(std::vector<double> util) { queue_util_ = std::move(util); }

    struct Report {
        std::string label;
        uint64_t logical_requests;
        uint64_t physical_ops;
        double coalescing_ratio;   // 1 - physical/logical
        double avg_latency_us;
        double p50_latency_us;
        double p95_latency_us;
        double p99_latency_us;
        double max_latency_us;
        double throughput_MBps;       // physical bytes actually written to the device
        double app_throughput_MBps;   // logical bytes the application asked to persist
        double iops;               // logical requests / second (app-visible)
        double physical_iops;      // physical ops / second (device-visible)
        double wall_seconds;
        std::vector<double> queue_utilization_pct;
        double avg_queue_utilization_pct;
    };

    Report compute(const std::string& label);
    static void print_report(const Report& r);

private:
    std::mutex mtx_;
    std::vector<double> latencies_us_;
    uint64_t logical_requests_ = 0;
    uint64_t physical_ops_ = 0;
    uint64_t bytes_written_ = 0;
    uint64_t logical_bytes_ = 0;
    double wall_seconds_ = 0.0;
    std::vector<double> queue_util_;
};

} // namespace nvmesim

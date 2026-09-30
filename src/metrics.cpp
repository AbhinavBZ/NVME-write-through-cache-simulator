#include "metrics.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>

namespace nvmesim {

void MetricsCollector::record_latency_us(double us) {
    std::lock_guard<std::mutex> lk(mtx_);
    latencies_us_.push_back(us);
}

static double percentile(std::vector<double>& sorted, double p) {
    if (sorted.empty()) return 0.0;
    double idx = p * (sorted.size() - 1);
    size_t lo = static_cast<size_t>(std::floor(idx));
    size_t hi = static_cast<size_t>(std::ceil(idx));
    if (lo == hi) return sorted[lo];
    double frac = idx - lo;
    return sorted[lo] * (1 - frac) + sorted[hi] * frac;
}

MetricsCollector::Report MetricsCollector::compute(const std::string& label) {
    std::lock_guard<std::mutex> lk(mtx_);
    Report r;
    r.label = label;
    r.logical_requests = logical_requests_;
    r.physical_ops = physical_ops_;
    r.coalescing_ratio = logical_requests_ > 0
        ? 1.0 - (static_cast<double>(physical_ops_) / static_cast<double>(logical_requests_))
        : 0.0;
    r.wall_seconds = wall_seconds_;

    std::vector<double> sorted = latencies_us_;
    std::sort(sorted.begin(), sorted.end());
    if (!sorted.empty()) {
        r.avg_latency_us = std::accumulate(sorted.begin(), sorted.end(), 0.0) / sorted.size();
        r.p50_latency_us = percentile(sorted, 0.50);
        r.p95_latency_us = percentile(sorted, 0.95);
        r.p99_latency_us = percentile(sorted, 0.99);
        r.max_latency_us = sorted.back();
    } else {
        r.avg_latency_us = r.p50_latency_us = r.p95_latency_us = r.p99_latency_us = r.max_latency_us = 0.0;
    }

    r.throughput_MBps = wall_seconds_ > 0
        ? (bytes_written_ / (1024.0 * 1024.0)) / wall_seconds_ : 0.0;
    r.app_throughput_MBps = wall_seconds_ > 0
        ? (logical_bytes_ / (1024.0 * 1024.0)) / wall_seconds_ : 0.0;
    r.iops = wall_seconds_ > 0 ? logical_requests_ / wall_seconds_ : 0.0;
    r.physical_iops = wall_seconds_ > 0 ? physical_ops_ / wall_seconds_ : 0.0;

    r.queue_utilization_pct = queue_util_;
    r.avg_queue_utilization_pct = queue_util_.empty() ? 0.0
        : std::accumulate(queue_util_.begin(), queue_util_.end(), 0.0) / queue_util_.size();

    return r;
}

void MetricsCollector::print_report(const Report& r) {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n===== " << r.label << " =====\n";
    std::cout << "  Logical write requests   : " << r.logical_requests << "\n";
    std::cout << "  Physical storage ops     : " << r.physical_ops << "\n";
    std::cout << "  Coalescing reduction     : " << (r.coalescing_ratio * 100.0) << " %\n";
    std::cout << "  Wall-clock time          : " << r.wall_seconds << " s\n";
    std::cout << "  Avg latency              : " << r.avg_latency_us << " us\n";
    std::cout << "  p50 latency              : " << r.p50_latency_us << " us\n";
    std::cout << "  p95 latency              : " << r.p95_latency_us << " us\n";
    std::cout << "  p99 latency              : " << r.p99_latency_us << " us\n";
    std::cout << "  Max latency              : " << r.max_latency_us << " us\n";
    std::cout << "  Physical throughput      : " << r.throughput_MBps << " MB/s (bytes actually written to device)\n";
    std::cout << "  Application throughput   : " << r.app_throughput_MBps << " MB/s (bytes the app asked to persist)\n";
    std::cout << "  Application IOPS         : " << r.iops << "\n";
    std::cout << "  Physical device IOPS     : " << r.physical_iops << "\n";
    std::cout << "  Avg queue utilization    : " << r.avg_queue_utilization_pct << " %\n";
    std::cout << "  Per-queue utilization    : ";
    for (double u : r.queue_utilization_pct) std::cout << u << "% ";
    std::cout << "\n";
}

} // namespace nvmesim

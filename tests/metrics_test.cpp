#include "metrics.hpp"

#include <cassert>
#include <iostream>

using namespace nvmesim;

int main() {
    MetricsCollector metrics;

    metrics.set_logical_requests(100);
    metrics.set_physical_ops(80);
    metrics.set_wall_seconds(1.0);

    // Add deterministic latency samples: 1 through 100 microseconds.
    for (int i = 1; i <= 100; ++i) {
        metrics.record_latency_us(static_cast<double>(i));
    }

    const auto report = metrics.compute("Metrics Test");

    // Request and operation counts.
    assert(report.logical_requests == 100);
    assert(report.physical_ops == 80);


    // Latency statistics.
    assert(std::abs(report.avg_latency_us - 50.5) < 0.001);
    assert(report.p50_latency_us >= 50.0);
    assert(report.p95_latency_us >= 95.0);
    assert(report.p99_latency_us >= 99.0);
    assert(std::abs(report.max_latency_us - 100.0) < 0.001);

    // Basic derived performance metrics.
    assert(report.iops > 0.0);
    assert(report.throughput_MBps >= 0.0);

    std::cout << "Metrics collector tests: PASS\n";

    return 0;
}

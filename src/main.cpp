// main.cpp
// Drives the same synthetic write workload through the baseline (naive)
// and optimized write-through caches over the same NVMe device model,
// then prints a side-by-side comparison and dumps a CSV for plotting.
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "metrics.hpp"
#include "nvme_device.hpp"
#include "workload.hpp"
#include "write_through_cache.hpp"

using namespace nvmesim;

struct Config {
    uint64_t num_requests = 20000;
    uint64_t address_space = 50000;   // blocks
    size_t block_size = 4096;
    int num_queues = 8;
    Distribution dist = Distribution::Zipfian;
    double zipf_skew = 1.2;
    int flush_interval_us = 200;
    size_t batch_trigger = 64;
    int app_threads = 8;
    uint32_t seed = 42;
    std::string backing_dir = "/tmp";
    std::string results_dir = "results";
    uint64_t sim_latency_ns = 60000; // 60us modeled channel latency; 0 = use real O_DIRECT timing
};

static void parse_args(int argc, char** argv, Config& cfg) {
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) { std::cerr << "missing value for " << flag << "\n"; exit(1); }
            return argv[++i];
        };
        if (a == "--requests") cfg.num_requests = std::stoull(next("--requests"));
        else if (a == "--address-space") cfg.address_space = std::stoull(next("--address-space"));
        else if (a == "--queues") cfg.num_queues = std::stoi(next("--queues"));
        else if (a == "--distribution") {
            std::string v = next("--distribution");
            cfg.dist = (v == "uniform") ? Distribution::Uniform : Distribution::Zipfian;
        } else if (a == "--zipf-skew") cfg.zipf_skew = std::stod(next("--zipf-skew"));
        else if (a == "--flush-interval-us") cfg.flush_interval_us = std::stoi(next("--flush-interval-us"));
        else if (a == "--batch-trigger") cfg.batch_trigger = std::stoull(next("--batch-trigger"));
        else if (a == "--app-threads") cfg.app_threads = std::stoi(next("--app-threads"));
        else if (a == "--seed") cfg.seed = static_cast<uint32_t>(std::stoul(next("--seed")));
        else if (a == "--sim-latency-us") cfg.sim_latency_ns = std::stoull(next("--sim-latency-us")) * 1000ULL;
        else if (a == "--backing-dir") cfg.backing_dir = next("--backing-dir");
        else if (a == "--results-dir") cfg.results_dir = next("--results-dir");
        else if (a == "--help") {
            std::cout << "Usage: nvme_wt_sim [options]\n"
                      << "  --requests N          number of logical write requests (default 20000)\n"
                      << "  --address-space N     device address space in blocks (default 50000)\n"
                      << "  --queues N            number of NVMe queue pairs (default 8)\n"
                      << "  --distribution uniform|zipf  (default zipf)\n"
                      << "  --zipf-skew F         zipf skew factor (default 1.2)\n"
                      << "  --flush-interval-us N optimized-cache flush period (default 200)\n"
                      << "  --batch-trigger N     pending writes that force an early flush (default 64)\n"
                      << "  --app-threads N       concurrent application threads for optimized run (default 8)\n"
                      << "  --seed N              RNG seed, shared by both runs (default 42)\n"
                      << "  --sim-latency-us N    modeled per-op device latency in us; 0 = use real\n"
                      << "                        O_DIRECT pwrite() timing instead (default 60)\n"
                      << "  --backing-dir DIR     where to put the scratch backing files (default /tmp)\n"
                      << "  --results-dir DIR     where to write comparison.csv (default results)\n";
            exit(0);
        } else {
            std::cerr << "unknown argument: " << a << "\n"; exit(1);
        }
    }
}

static std::vector<char> make_value(size_t block_size, uint64_t lba) {
    std::vector<char> v(block_size);
    char fill = static_cast<char>((lba % 251) + 1);
    std::memset(v.data(), fill, v.size());
    return v;
}

static MetricsCollector::Report run_baseline(const Config& cfg,
                                              const std::vector<WorkloadRequest>& workload) {
    NVMeDevice device(cfg.backing_dir + "/nvme_sim_baseline.bin", cfg.address_space,
                       cfg.block_size, cfg.num_queues, cfg.sim_latency_ns);
    MetricsCollector metrics;
    BaselineWriteThroughCache cache(device, metrics);

    // Same offered concurrency as the optimized run: this is the fair,
    // apples-to-apples comparison. The naive cache still funnels every
    // write through one queue at queue depth 1 (see write_through_cache.cpp),
    // so under concurrent demand from many application threads those
    // writes queue up behind each other -- exactly the software
    // bottleneck the problem statement describes, and exactly what a
    // single-threaded baseline run would hide.
    int nthreads = std::max(1, cfg.app_threads);
    std::vector<std::thread> threads;
    size_t chunk = (workload.size() + nthreads - 1) / nthreads;

    auto t0 = Clock::now();
    for (int t = 0; t < nthreads; ++t) {
        size_t begin = std::min(workload.size(), (size_t)t * chunk);
        size_t end = std::min(workload.size(), begin + chunk);
        if (begin >= end) continue;
        threads.emplace_back([&, begin, end] {
            for (size_t i = begin; i < end; ++i) {
                const auto& req = workload[i];
                cache.write(req.lba, make_value(cfg.block_size, req.lba));
            }
        });
    }
    for (auto& th : threads) th.join();
    auto t1 = Clock::now();
    double wall = std::chrono::duration<double>(t1 - t0).count();

    metrics.set_logical_requests(workload.size());
    metrics.set_physical_ops(device.total_physical_ops());
    metrics.set_bytes_written(device.total_bytes_written());
    metrics.set_logical_bytes_requested(workload.size() * cfg.block_size);
    metrics.set_wall_seconds(wall);
    metrics.set_queue_utilization(device.per_queue_utilization(wall));

    return metrics.compute("Baseline: naive synchronous write-through (1 queue, queue depth 1)");
}

static MetricsCollector::Report run_optimized(const Config& cfg,
                                               const std::vector<WorkloadRequest>& workload) {
    NVMeDevice device(cfg.backing_dir + "/nvme_sim_optimized.bin", cfg.address_space,
                       cfg.block_size, cfg.num_queues, cfg.sim_latency_ns);
    MetricsCollector metrics;
    OptimizedWriteThroughCache cache(device, metrics,
                                      std::chrono::microseconds(cfg.flush_interval_us),
                                      cfg.batch_trigger);
    cache.start();

    int nthreads = std::max(1, cfg.app_threads);
    std::vector<std::thread> threads;
    size_t chunk = (workload.size() + nthreads - 1) / nthreads;

    auto t0 = Clock::now();
    for (int t = 0; t < nthreads; ++t) {
        size_t begin = std::min(workload.size(), (size_t)t * chunk);
        size_t end = std::min(workload.size(), begin + chunk);
        if (begin >= end) continue;
        threads.emplace_back([&, begin, end] {
            for (size_t i = begin; i < end; ++i) {
                const auto& req = workload[i];
                cache.write(req.lba, make_value(cfg.block_size, req.lba));
            }
        });
    }
    for (auto& th : threads) th.join();
    cache.stop(); // flushes any remaining coalesced writes
    auto t1 = Clock::now();
    double wall = std::chrono::duration<double>(t1 - t0).count();

    metrics.set_logical_requests(workload.size());
    metrics.set_physical_ops(device.total_physical_ops());
    metrics.set_bytes_written(device.total_bytes_written());
    metrics.set_logical_bytes_requested(workload.size() * cfg.block_size);
    metrics.set_wall_seconds(wall);
    metrics.set_queue_utilization(device.per_queue_utilization(wall));

    return metrics.compute("Optimized: coalesced + batched + multi-queue write-through");
}

static void print_comparison(const MetricsCollector::Report& base, const MetricsCollector::Report& opt) {
    auto speedup = [](double b, double o) { return o > 0 ? b / o : 0.0; };
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n================= COMPARISON =================\n";
    std::cout << "Metric                     Baseline        Optimized       Improvement\n";
    std::cout << "Avg latency (us)           " << std::setw(10) << base.avg_latency_us
              << "      " << std::setw(10) << opt.avg_latency_us
              << "      " << speedup(base.avg_latency_us, opt.avg_latency_us) << "x lower\n";
    std::cout << "p99 latency (us)           " << std::setw(10) << base.p99_latency_us
              << "      " << std::setw(10) << opt.p99_latency_us
              << "      " << speedup(base.p99_latency_us, opt.p99_latency_us) << "x lower\n";
    std::cout << "Physical throughput (MB/s) " << std::setw(10) << base.throughput_MBps
              << "      " << std::setw(10) << opt.throughput_MBps
              << "      " << speedup(opt.throughput_MBps, base.throughput_MBps) << "x higher\n";
    std::cout << "App throughput (MB/s)      " << std::setw(10) << base.app_throughput_MBps
              << "      " << std::setw(10) << opt.app_throughput_MBps
              << "      " << speedup(opt.app_throughput_MBps, base.app_throughput_MBps) << "x higher\n";
    std::cout << "Application IOPS           " << std::setw(10) << base.iops
              << "      " << std::setw(10) << opt.iops
              << "      " << speedup(opt.iops, base.iops) << "x higher\n";
    std::cout << "Physical device ops        " << std::setw(10) << base.physical_ops
              << "      " << std::setw(10) << opt.physical_ops
              << "      " << (base.physical_ops > 0
                                ? 100.0 * (1.0 - (double)opt.physical_ops / base.physical_ops)
                                : 0.0) << "% fewer\n";
    std::cout << "Avg queue utilization (%)  " << std::setw(10) << base.avg_queue_utilization_pct
              << "      " << std::setw(10) << opt.avg_queue_utilization_pct << "\n";
    std::cout << "================================================\n";
}

static void write_csv(const std::string& path, const MetricsCollector::Report& base,
                       const MetricsCollector::Report& opt) {
    std::ofstream f(path);
    f << "metric,baseline,optimized\n";
    f << "logical_requests," << base.logical_requests << "," << opt.logical_requests << "\n";
    f << "physical_ops," << base.physical_ops << "," << opt.physical_ops << "\n";
    f << "coalescing_ratio_pct," << base.coalescing_ratio * 100 << "," << opt.coalescing_ratio * 100 << "\n";
    f << "avg_latency_us," << base.avg_latency_us << "," << opt.avg_latency_us << "\n";
    f << "p50_latency_us," << base.p50_latency_us << "," << opt.p50_latency_us << "\n";
    f << "p95_latency_us," << base.p95_latency_us << "," << opt.p95_latency_us << "\n";
    f << "p99_latency_us," << base.p99_latency_us << "," << opt.p99_latency_us << "\n";
    f << "max_latency_us," << base.max_latency_us << "," << opt.max_latency_us << "\n";
    f << "physical_throughput_MBps," << base.throughput_MBps << "," << opt.throughput_MBps << "\n";
    f << "app_throughput_MBps," << base.app_throughput_MBps << "," << opt.app_throughput_MBps << "\n";
    f << "app_iops," << base.iops << "," << opt.iops << "\n";
    f << "physical_iops," << base.physical_iops << "," << opt.physical_iops << "\n";
    f << "wall_seconds," << base.wall_seconds << "," << opt.wall_seconds << "\n";
    f << "avg_queue_utilization_pct," << base.avg_queue_utilization_pct << "," << opt.avg_queue_utilization_pct << "\n";
    std::cout << "\nWrote comparison CSV to " << path << "\n";
}

int main(int argc, char** argv) {
    Config cfg;
    parse_args(argc, argv, cfg);

    std::cout << "NVMe Write-Through Caching Accelerator Simulator\n";
    std::cout << "requests=" << cfg.num_requests << " address_space=" << cfg.address_space
              << " queues=" << cfg.num_queues << " distribution="
              << (cfg.dist == Distribution::Zipfian ? "zipf" : "uniform")
              << " zipf_skew=" << cfg.zipf_skew << " flush_interval_us=" << cfg.flush_interval_us
              << " batch_trigger=" << cfg.batch_trigger << " app_threads=" << cfg.app_threads
              << " seed=" << cfg.seed << " sim_latency_us="
              << (cfg.sim_latency_ns == 0 ? std::string("real-O_DIRECT-timing")
                                           : std::to_string(cfg.sim_latency_ns / 1000)) << "\n";

    WorkloadGenerator gen(cfg.address_space, cfg.num_requests, cfg.dist, cfg.zipf_skew, cfg.seed);
    std::vector<WorkloadRequest> workload = gen.generate();

    auto base_report = run_baseline(cfg, workload);
    MetricsCollector::print_report(base_report);

    auto opt_report = run_optimized(cfg, workload);
    MetricsCollector::print_report(opt_report);

    print_comparison(base_report, opt_report);
    write_csv(cfg.results_dir + "/comparison.csv", base_report, opt_report);

    return 0;
}

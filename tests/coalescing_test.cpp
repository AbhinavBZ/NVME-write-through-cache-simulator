#include "metrics.hpp"
#include "nvme_device.hpp"
#include "write_through_cache.hpp"

#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace nvmesim;

namespace {

std::vector<char> read_block(const std::string& path,
                             std::uint64_t lba,
                             std::size_t block_size) {
    std::ifstream file(path, std::ios::binary);

    assert(file.is_open());

    file.seekg(static_cast<std::streamoff>(lba * block_size));

    std::vector<char> data(block_size);
    file.read(data.data(), static_cast<std::streamsize>(data.size()));

    assert(file.gcount() == static_cast<std::streamsize>(block_size));

    return data;
}

} // namespace

int main() {
    constexpr std::size_t block_size = 4096;
    constexpr std::uint64_t device_blocks = 32;
    constexpr int queue_count = 2;

    // Enough device latency to keep concurrent writes overlapping.
    constexpr std::uint64_t simulated_latency_ns = 1'000'000; // 1 ms

    constexpr std::uint64_t test_lba = 10;
    constexpr int thread_count = 8;

    const std::string backing_path =
        "/tmp/nvme_wt_cache_coalescing_test.bin";

    MetricsCollector metrics;

    NVMeDevice device(
        backing_path,
        device_blocks,
        block_size,
        queue_count,
        simulated_latency_ns
    );

    OptimizedWriteThroughCache cache(
        device,
        metrics,
        std::chrono::microseconds(10'000), // 10 ms
        64
    );

    cache.start();

    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    for (int i = 0; i < thread_count; ++i) {
        workers.emplace_back([&, i]() {
            std::vector<char> value(
                block_size,
                static_cast<char>('A' + i)
            );

            cache.write(test_lba, value);
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    cache.stop();

    const auto stored = read_block(
        backing_path,
        test_lba,
        block_size
    );

    // The final durable value must come from one of the submitted writes.
    for (char byte : stored) {
        assert(byte >= 'A' && byte < 'A' + thread_count);
    }

    const auto physical_ops = device.total_physical_ops();

    assert(
        physical_ops < static_cast<std::uint64_t>(thread_count)
    );

    std::cout << "Coalescing test: PASS\n";
    std::cout << "Logical writes: " << thread_count << '\n';
    std::cout << "Physical writes: " << physical_ops << '\n';

    return 0;
}

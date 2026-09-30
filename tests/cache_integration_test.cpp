#include "metrics.hpp"
#include "nvme_device.hpp"
#include "write_through_cache.hpp"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
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

}  // namespace

int main() {
    constexpr std::size_t block_size = 4096;
    constexpr std::uint64_t device_blocks = 32;
    constexpr int queue_count = 2;
    constexpr std::uint64_t simulated_latency_ns = 0;

    const std::string backing_path = "/tmp/nvme_wt_cache_integration_test.bin";

    // ------------------------------------------------------------
    // Baseline write-through cache test.
    // ------------------------------------------------------------

    {
        MetricsCollector metrics;

        NVMeDevice device(
            backing_path,
            device_blocks,
            block_size,
            queue_count,
            simulated_latency_ns
        );

        BaselineWriteThroughCache cache(device, metrics);

        const std::uint64_t lba = 4;
        const std::vector<char> value(block_size, 'B');

        cache.write(lba, value);

        const auto stored = read_block(backing_path, lba, block_size);

        assert(stored == value);
        assert(device.total_physical_ops() == 1);
    }

    // The NVMeDevice destructor removes the backing file.
    std::remove(backing_path.c_str());

    // ------------------------------------------------------------
    // Optimized write-through cache test.
    // ------------------------------------------------------------

    {
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
            std::chrono::microseconds(100),
            4
        );

        cache.start();

        const std::uint64_t lba = 8;
        const std::vector<char> value(block_size, 'O');

        cache.write(lba, value);

        const auto stored = read_block(backing_path, lba, block_size);

        assert(stored == value);
        assert(device.total_physical_ops() >= 1);

        cache.stop();
    }

    std::remove(backing_path.c_str());

    std::cout << "Cache/device integration tests: PASS\n";

    return 0;
}

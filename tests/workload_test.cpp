#include "workload.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace nvmesim;

int main() {
    constexpr std::size_t request_count = 1000;
    constexpr std::uint64_t address_space = 50000;

    // Test uniform workload generation.
    WorkloadGenerator uniform_generator(
        address_space,
        request_count,
        Distribution::Uniform,
        1.2,
        42
    );

    auto uniform_requests = uniform_generator.generate();

    assert(uniform_requests.size() == request_count);

    for (const auto& request : uniform_requests) {
        assert(request.lba < address_space);
        assert(request.num_blocks > 0);
    }

    // Test Zipf workload generation.
    WorkloadGenerator zipf_generator(
        address_space,
        request_count,
        Distribution::Zipfian,
        1.5,
        42
    );

    auto zipf_requests = zipf_generator.generate();

    assert(zipf_requests.size() == request_count);

    for (const auto& request : zipf_requests) {
        assert(request.lba < address_space);
        assert(request.num_blocks > 0);
    }

    // Test deterministic generation with the same seed.
    WorkloadGenerator generator_a(
        address_space,
        100,
        Distribution::Uniform,
        1.2,
        123
    );

    WorkloadGenerator generator_b(
        address_space,
        100,
        Distribution::Uniform,
        1.2,
        123
    );

    const auto requests_a = generator_a.generate();
    const auto requests_b = generator_b.generate();

    assert(requests_a.size() == requests_b.size());

    for (std::size_t i = 0; i < requests_a.size(); ++i) {
        assert(requests_a[i].lba == requests_b[i].lba);
        assert(requests_a[i].num_blocks == requests_b[i].num_blocks);
    }

    std::cout << "Workload generator tests: PASS\n";

    return 0;
}

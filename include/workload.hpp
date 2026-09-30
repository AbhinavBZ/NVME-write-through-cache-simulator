// workload.hpp
// Generates a reproducible sequence of logical write requests (LBA to
// write to). A Zipfian ("hot address") distribution is included because
// write coalescing only pays off when some addresses are written
// repeatedly in a short window -- exactly the skew real workloads
// (metadata blocks, counters, journal heads) tend to show.
#pragma once

#include <cstdint>
#include <random>
#include <vector>

namespace nvmesim {

enum class Distribution { Uniform, Zipfian };

struct WorkloadRequest {
    uint64_t lba;
    uint32_t num_blocks;
};

class WorkloadGenerator {
public:
    WorkloadGenerator(uint64_t address_space_blocks, uint64_t num_requests,
                       Distribution dist, double zipf_skew, uint32_t seed);

    std::vector<WorkloadRequest> generate();

private:
    uint64_t address_space_;
    uint64_t num_requests_;
    Distribution dist_;
    double skew_;
    uint32_t seed_;
};

} // namespace nvmesim

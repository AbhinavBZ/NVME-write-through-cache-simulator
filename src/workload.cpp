#include "workload.hpp"

#include <cmath>

namespace nvmesim {

WorkloadGenerator::WorkloadGenerator(uint64_t address_space_blocks, uint64_t num_requests,
                                      Distribution dist, double zipf_skew, uint32_t seed)
    : address_space_(address_space_blocks), num_requests_(num_requests),
      dist_(dist), skew_(zipf_skew), seed_(seed) {}

std::vector<WorkloadRequest> WorkloadGenerator::generate() {
    std::vector<WorkloadRequest> out;
    out.reserve(num_requests_);
    std::mt19937_64 rng(seed_);

    if (dist_ == Distribution::Uniform) {
        std::uniform_int_distribution<uint64_t> pick(0, address_space_ - 1);
        for (uint64_t i = 0; i < num_requests_; ++i) {
            out.push_back({pick(rng), 1});
        }
    } else {
        // Build Zipfian weights over a "hot set" so a small number of LBAs
        // absorb a disproportionate share of writes (rank-based, skew_ > 1
        // means stronger skew). Capping the ranked set keeps setup cheap
        // for large address spaces while still modeling realistic skew.
        uint64_t ranked = std::min<uint64_t>(address_space_, 200000);
        std::vector<double> weights(ranked);
        for (uint64_t i = 0; i < ranked; ++i) {
            weights[i] = 1.0 / std::pow(static_cast<double>(i + 1), skew_);
        }
        std::discrete_distribution<uint64_t> pick(weights.begin(), weights.end());
        for (uint64_t i = 0; i < num_requests_; ++i) {
            out.push_back({pick(rng), 1});
        }
    }
    return out;
}

} // namespace nvmesim

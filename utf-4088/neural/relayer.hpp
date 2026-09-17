#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace utf4088 {

struct Field4D { double x, y, pressure, voltage; };
struct RelayerState { std::array<double, 16> hidden{}; std::uint64_t engram_hash{}; };
struct CharacterCandidate { std::uint64_t character_id{}; double score{}; };
struct RelayerResult { RelayerState state; std::vector<CharacterCandidate> candidates; std::uint64_t selected_id{}; };

// Deterministic feature synthesis. Model weights are supplied by the caller.
RelayerResult synthesize(const Field4D& input,
                         const RelayerState& prior,
                         const std::vector<std::array<double,16>>& model_weights,
                         const std::vector<std::uint64_t>& candidate_ids);

} // namespace utf4088

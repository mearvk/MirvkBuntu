#pragma once

#include <cstdint>
#include <vector>

namespace utf4088 {

struct SymbolPoint {
    double x;
    double y;
    double pressure;
    double voltage;
};

struct ResolutionScore {
    double shape_similarity;
    double graph_continuity;
    double field_uniformity;
    double language_fit;
    double total;
};

// Deterministic resolution score for a candidate symbol against a field path.
ResolutionScore resolve_symbol(const std::vector<SymbolPoint>& path,
                               const std::vector<SymbolPoint>& candidate);

} // namespace utf4088

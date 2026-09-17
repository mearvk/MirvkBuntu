#include "glyph8x12.hpp"
#include <algorithm>
#include <cstdint>

namespace utf4088 {

struct ResolutionScore {
    double score;
    GlyphMetrics metrics;
};

static ResolutionScore resolve_glyph(const Glyph8x12& candidate) {
    const auto m=analyze_glyph(candidate);
    const double density=static_cast<double>(m.black_pixels)/96.0;
    const double connectivity=m.connected?1.0:0.0;
    const double topology=std::min(1.0, static_cast<double>(m.edge_count)/95.0);
    const double complexity=std::min(1.0, static_cast<double>(m.transitions)/120.0);
    // Resolution score: connected, sparse-to-moderate, topologically expressive.
    const double score=0.45*connectivity + 0.25*topology + 0.20*complexity +
                       0.10*(1.0-std::abs(density-0.30));
    return {score,m};
}

} // namespace utf4088

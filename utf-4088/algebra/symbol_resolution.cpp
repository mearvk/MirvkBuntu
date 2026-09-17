#include "symbol_resolution.hpp"
#include <algorithm>
#include <cmath>

namespace utf4088 {

namespace {
double distance(const SymbolPoint& a, const SymbolPoint& b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    const double dp = a.pressure - b.pressure;
    const double dv = a.voltage - b.voltage;
    return std::sqrt(dx*dx + dy*dy + dp*dp + dv*dv);
}

double path_error(const std::vector<SymbolPoint>& a,
                  const std::vector<SymbolPoint>& b) {
    if (a.empty() || b.empty()) return 1.0;
    const std::size_t n = std::min(a.size(), b.size());
    double sum = 0.0;
    for (std::size_t i = 0; i < n; ++i) sum += distance(a[i], b[i]);
    return sum / static_cast<double>(n);
}
} // namespace

ResolutionScore resolve_symbol(const std::vector<SymbolPoint>& path,
                               const std::vector<SymbolPoint>& candidate) {
    const double error = path_error(path, candidate);
    const double shape = 1.0 / (1.0 + error);

    double continuity = 1.0;
    if (path.size() > 1) {
        double total = 0.0;
        for (std::size_t i = 1; i < path.size(); ++i) {
            const double dx = path[i].x - path[i-1].x;
            const double dy = path[i].y - path[i-1].y;
            total += std::hypot(dx, dy);
        }
        continuity = 1.0 / (1.0 + total / static_cast<double>(path.size()-1));
    }

    double variance = 0.0;
    if (!path.empty()) {
        double mean = 0.0;
        for (const auto& p : path) mean += p.voltage;
        mean /= static_cast<double>(path.size());
        for (const auto& p : path) {
            const double d = p.voltage - mean;
            variance += d*d;
        }
        variance /= static_cast<double>(path.size());
    }
    const double uniformity = 1.0 / (1.0 + std::sqrt(variance));

    // Language fit is deliberately neutral until a language-specific model
    // is supplied. This prevents the physical field from being treated as
    // intrinsically encoding a language or meaning.
    const double language_fit = 0.0;
    const double total = 0.40*shape + 0.25*continuity +
                         0.20*uniformity + 0.15*language_fit;
    return {shape, continuity, uniformity, language_fit, total};
}

} // namespace utf4088

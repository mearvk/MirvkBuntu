#include "digraph_engine.hpp"

#include <cmath>
#include <cstdint>
#include <unordered_map>

namespace utf4088 {

namespace {
std::uint64_t key(std::uint64_t a, std::uint64_t b) {
    // Stable 64-bit pair mixing for graph indexing.
    a ^= a >> 30;
    a *= 0xbf58476d1ce4e5b9ULL;
    a ^= a >> 27;
    a *= 0x94d049bb133111ebULL;
    a ^= a >> 31;
    b ^= b >> 30;
    b *= 0xbf58476d1ce4e5b9ULL;
    b ^= b >> 27;
    b *= 0x94d049bb133111ebULL;
    b ^= b >> 31;
    return a ^ (b + 0x9e3779b97f4a7c15ULL + (a << 6) + (a >> 2));
}

std::uint64_t input_id(const FieldState& s) {
    auto q = [](double v) -> std::uint64_t {
        if (!std::isfinite(v)) return 0;
        const double a = std::min(std::abs(v), 1.0e9) * 1000.0;
        return static_cast<std::uint64_t>(a);
    };
    return (q(s.voltage) & 0xFFFFFULL) << 44 |
           (q(s.magnitude) & 0xFFFFFULL) << 24 |
           (q(s.uniformity) & 0xFFFFFULL) << 4 |
           (q(s.direction) & 0xFULL);
}
} // namespace

ConceptGraph build_digraph(const std::vector<FieldState>& states) {
    ConceptGraph graph;
    std::unordered_map<std::uint64_t, std::size_t> nodes;
    std::unordered_map<std::uint64_t, std::size_t> edges;

    for (const auto& state : states) {
        const auto id = input_id(state);
        if (!nodes.contains(id)) {
            nodes.emplace(id, graph.nodes.size());
            graph.nodes.push_back({id, id, 1.0});
        } else {
            graph.nodes[nodes[id]].weight += 1.0;
        }
    }

    for (std::size_t i = 1; i < states.size(); ++i) {
        const auto from = input_id(states[i - 1]);
        const auto to = input_id(states[i]);
        const auto k = key(from, to);

        if (!edges.contains(k)) {
            edges.emplace(k, graph.edges.size());
            graph.edges.push_back({from, to, 1.0, states[i].direction});
        } else {
            auto& edge = graph.edges[edges[k]];
            edge.weight += 1.0;
            edge.direction = (edge.direction + states[i].direction) * 0.5;
        }
    }

    return graph;
}

} // namespace utf4088

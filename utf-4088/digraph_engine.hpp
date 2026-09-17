#pragma once

#include "voltage_field.hpp"
#include <cstdint>
#include <vector>

namespace utf4088 {

struct GraphNode {
    std::uint64_t id;
    std::uint64_t symbol_input;
    double weight;
};

struct GraphEdge {
    std::uint64_t from;
    std::uint64_t to;
    double weight;
    double direction;
};

struct ConceptGraph {
    std::vector<GraphNode> nodes;
    std::vector<GraphEdge> edges;
};

// Build a directed graph from consecutive field states. Each adjacent pair
// produces one directed digraph edge; identical inputs are coalesced.
ConceptGraph build_digraph(const std::vector<FieldState>& states);

} // namespace utf4088

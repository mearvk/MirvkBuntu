#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace utf4088 {

struct VoltageSample {
    double x;
    double y;
    double volts;
    double time;
};

struct FieldState {
    double voltage;
    double dVdx;
    double dVdy;
    double magnitude;
    double direction;
    double uniformity;
};

// Analyze a 2-D field around a sample using finite differences.
FieldState analyze_field(const std::vector<VoltageSample>& samples,
                         std::size_t center_index);

// Map the measured field state into a deterministic symbolic driver input.
std::uint64_t field_to_input(const FieldState& state);

} // namespace utf4088

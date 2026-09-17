#pragma once

#include <cstdint>
#include <optional>

namespace utf4088 {

using InputState = std::uint64_t;
using CodePoint = std::uint64_t;

// Returns a UTF-4088 experimental code point for a documented digital input.
std::optional<CodePoint> symbol_from_input(InputState input);

// True when a value is in the proposed >32-bit experimental code space.
bool is_valid_code_point(CodePoint value);

} // namespace utf4088

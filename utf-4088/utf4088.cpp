#include "utf4088.hpp"

namespace utf4088 {

namespace {
constexpr CodePoint kMaxCodePoint = (CodePoint{1} << 32) + 0xFFFFFFFFULL;
}

bool is_valid_code_point(CodePoint value) {
    // Experimental range: values above the UTF-32 ceiling through the
    // proposed 4088-bit conceptual space are implementation-defined.
    return value > 0x10FFFFULL && value <= kMaxCodePoint;
}

std::optional<CodePoint> symbol_from_input(InputState input) {
    // The initial driver is deliberately deterministic and digital. It does
    // not sample raw electrical voltage. Hardware adapters must convert
    // electrical signals into a documented digital InputState first.
    const CodePoint candidate = static_cast<CodePoint>(input);
    if (!is_valid_code_point(candidate)) {
        return std::nullopt;
    }
    return candidate;
}

} // namespace utf4088

#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace utf4088 {

struct Glyph8x12 {
    std::array<std::uint8_t, 12> rows{}; // low 8 bits are pixels; 1 = black
};

struct GlyphMetrics {
    std::uint16_t black_pixels;
    std::uint16_t connected_components;
    std::uint16_t edge_count;
    std::uint16_t transitions;
    bool connected;
    std::uint32_t signature;
};

GlyphMetrics analyze_glyph(const Glyph8x12& glyph);
Glyph8x12 seed_glyph(std::string_view seed);
std::uint32_t glyph_signature(const Glyph8x12& glyph);

} // namespace utf4088

#include "character_map.hpp"

#include <cmath>
#include <limits>

namespace utf4088 {

namespace {
std::uint64_t mix(std::uint64_t z) {
    z ^= z >> 30;
    z *= 0xbf58476d1ce4e5b9ULL;
    z ^= z >> 27;
    z *= 0x94d049bb133111ebULL;
    z ^= z >> 31;
    return z;
}

std::uint64_t q(double value) {
    if (!std::isfinite(value)) return 0;
    const double bounded = std::clamp(value, -1.0e9, 1.0e9);
    const auto scaled = static_cast<std::int64_t>(std::llround(bounded * 1000000.0));
    return static_cast<std::uint64_t>(scaled);
}
} // namespace

const std::vector<CharacterRecord>& frontend_registry() {
    // This intentionally fails closed until the generated registry is
    // populated. A partial table must never masquerade as the promised 16,606
    // published-symbol front end.
    static const std::vector<CharacterRecord> registry;
    return registry;
}

std::uint64_t derive_remainder_symbol(double x, double y,
                                      double pressure, double voltage) {
    // 4-D deterministic address mixing. The output is an experimental symbol
    // identifier, not an assertion that every generated identifier already
    // has a human-language meaning or glyph.
    std::uint64_t h = 0x9e3779b97f4a7c15ULL;
    h ^= mix(q(x) + 0x100000001b3ULL);
    h ^= mix(q(y) + 0x9e3779b97f4a7c15ULL);
    h ^= mix(q(pressure) + 0xbf58476d1ce4e5b9ULL);
    h ^= mix(q(voltage) + 0x94d049bb133111ebULL);
    return mix(h);
}

std::string_view stage_name(Stage stage) {
    switch (stage) {
        case Stage::Start: return "start";
        case Stage::Intermediate: return "intermediate";
        case Stage::Final: return "final";
    }
    return "unknown";
}

std::string_view language_name(Language language) {
    switch (language) {
        case Language::AmericanEnglish: return "american-english";
        case Language::Korean: return "korean";
        case Language::Germanic: return "germanic";
    }
    return "unknown";
}

} // namespace utf4088

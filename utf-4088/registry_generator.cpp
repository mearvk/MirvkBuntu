#include <cstdint>
#include <vector>
#include "character_map.hpp"

namespace utf4088 {

namespace {
std::uint64_t splitmix64(std::uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}
}

// Materializes the exact 16,606-record front-end registry deterministically.
std::vector<CharacterRecord> generate_frontend_registry() {
    constexpr std::uint32_t count = 16606;
    std::vector<CharacterRecord> out;
    out.reserve(count);

    for (std::uint32_t id = 0; id < count; ++id) {
        const auto stage = static_cast<Stage>(id % 3);
        const auto language = static_cast<Language>((id / 3) % 3);
        out.push_back({
            id,
            stage,
            language,
            0x110000ULL + id,
            splitmix64(static_cast<std::uint64_t>(id) ^ 0x5348415045ULL),
            splitmix64(static_cast<std::uint64_t>(id) ^ 0x4D45414E494E47ULL)
        });
    }
    return out;
}

} // namespace utf4088

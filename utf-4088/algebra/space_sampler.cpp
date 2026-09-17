#include "glyph8x12.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

namespace {
struct Stats { std::uint64_t samples{}, nonempty{}, connected{}, sparse_connected{}, unique{}; };

std::uint64_t mix(std::uint64_t x) {
    x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27; x *= 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

std::array<std::uint32_t,3> sample96(std::uint64_t& state) {
    std::array<std::uint32_t,3> a{};
    for (auto& x : a) { state = mix(state); x = static_cast<std::uint32_t>(state); }
    return a;
}

utf4088::Glyph8x12 to_glyph(const std::array<std::uint32_t,3>& a) {
    utf4088::Glyph8x12 g{};
    for (int y=0; y<12; ++y) {
        const int word=y/4, shift=(y%4)*8;
        g.rows[y]=static_cast<std::uint8_t>((a[word] >> shift) & 0xffU);
    }
    return g;
}
}

int main(int argc, char** argv) {
    const std::uint64_t samples = argc > 1 ? std::stoull(argv[1]) : 1000000ULL;
    const std::string output = argc > 2 ? argv[2] : "utf4088-space-sample.csv";
    std::uint64_t state = 0x40880812ULL;
    Stats s{};
    std::ofstream out(output);
    if (!out) return 2;
    out << "sample,black_pixels,components,connected,edges,transitions,signature\n";
    for (std::uint64_t i=0; i<samples; ++i) {
        const auto bits=sample96(state);
        const auto glyph=to_glyph(bits);
        const auto m=utf4088::analyze_glyph(glyph);
        ++s.samples;
        if (m.black_pixels) ++s.nonempty;
        if (m.connected) ++s.connected;
        if (m.connected && m.black_pixels >= 4 && m.black_pixels <= 48) ++s.sparse_connected;
        ++s.unique;
        out << i << ',' << m.black_pixels << ',' << m.connected_components << ','
            << (m.connected ? 1 : 0) << ',' << m.edge_count << ','
            << m.transitions << ',' << m.signature << '\n';
    }
    std::cerr << "samples=" << s.samples << "\n"
              << "nonempty=" << s.nonempty << "\n"
              << "connected=" << s.connected << "\n"
              << "sparse_connected=" << s.sparse_connected << "\n"
              << "sampled_unique_states=" << s.unique << "\n";
    return 0;
}

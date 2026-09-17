#include "glyph8x12.hpp"
#include <array>
#include <queue>

namespace utf4088 {

namespace {
std::uint32_t mix(std::uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352dU;
    x ^= x >> 15; x *= 0x846ca68bU;
    return x ^ (x >> 16);
}
}

GlyphMetrics analyze_glyph(const Glyph8x12& g) {
    GlyphMetrics m{};
    std::array<bool,96> seen{};
    const auto on = [&](int x, int y) { return x >= 0 && x < 8 && y >= 0 && y < 12 && ((g.rows[y] >> x) & 1U); };

    std::uint32_t sig = 2166136261U;
    for (int y=0; y<12; ++y) for (int x=0; x<8; ++x) {
        const bool p = on(x,y);
        if (p) ++m.black_pixels;
        sig ^= p ? 1U : 0U; sig *= 16777619U;
        if (p) {
            if (x < 7 && on(x+1,y)) ++m.edge_count;
            if (y < 11 && on(x,y+1)) ++m.edge_count;
        }
        if (x < 7 && p != on(x+1,y)) ++m.transitions;
        if (y < 11 && p != on(x,y+1)) ++m.transitions;
    }

    for (int y=0; y<12; ++y) for (int x=0; x<8; ++x) {
        const int i=y*8+x;
        if (!on(x,y) || seen[i]) continue;
        ++m.connected_components; seen[i]=true;
        std::queue<int> q; q.push(i);
        while (!q.empty()) {
            const int n=q.front(); q.pop();
            const int cx=n%8, cy=n/8;
            constexpr int dx[4]={1,-1,0,0};
            constexpr int dy[4]={0,0,1,-1};
            for (int k=0;k<4;++k) {
                const int nx=cx+dx[k], ny=cy+dy[k];
                if (nx>=0&&nx<8&&ny>=0&&ny<12&&on(nx,ny)) {
                    const int ni=ny*8+nx;
                    if (!seen[ni]) { seen[ni]=true; q.push(ni); }
                }
            }
        }
    }
    m.connected = (m.black_pixels == 0) || (m.connected_components == 1);
    m.signature = mix(sig ^ (static_cast<std::uint32_t>(m.edge_count)<<16) ^ m.transitions);
    return m;
}

Glyph8x12 seed_glyph(std::string_view seed) {
    Glyph8x12 g{};
    std::uint32_t h=2166136261U;
    for (unsigned char c: seed) { h ^= c; h *= 16777619U; }
    for (int y=0;y<12;++y) {
        std::uint8_t row=0;
        for (int x=0;x<8;++x) {
            h = mix(h + static_cast<std::uint32_t>(y*8+x));
            if ((h >> 31) & 1U) row |= static_cast<std::uint8_t>(1U<<x);
        }
        g.rows[y]=row;
    }
    return g;
}

std::uint32_t glyph_signature(const Glyph8x12& g) { return analyze_glyph(g).signature; }

} // namespace utf4088

#include "glyph8x12.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

namespace {
std::uint64_t mix(std::uint64_t x) { x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL; x ^= x >> 27; x *= 0x94d049bb133111ebULL; return x ^ (x >> 31); }
std::array<std::uint32_t,3> sample96(std::uint64_t& s) { std::array<std::uint32_t,3> a{}; for (auto& x:a){s=mix(s);x=static_cast<std::uint32_t>(s);} return a; }
utf4088::Glyph8x12 to_glyph(const std::array<std::uint32_t,3>& a) { utf4088::Glyph8x12 g{}; for(int y=0;y<12;++y){int w=y/4,sh=(y%4)*8;g.rows[y]=static_cast<std::uint8_t>((a[w]>>sh)&255U);} return g; }
}
int main(int argc,char**argv){
 const std::uint64_t n=argc>1?std::stoull(argv[1]):5000000ULL; const std::string file=argc>2?argv[2]:"utf4088-interpretability-sample.csv";
 std::uint64_t state=0x40880812ULL; std::uint64_t connected=0, balanced=0, candidate=0, symmetric=0;
 std::ofstream out(file); if(!out){std::cerr<<"cannot open output\n";return 2;}
 out<<"sample,black_pixels,components,connected,edges,transitions,signature,balanced,candidate\n";
 for(std::uint64_t i=0;i<n;++i){auto g=to_glyph(sample96(state));auto m=utf4088::analyze_glyph(g);const bool b=m.black_pixels>=8&&m.black_pixels<=40;const bool c=m.connected&&b&&m.transitions>=8&&m.transitions<=90;const bool s=m.connected&&m.edge_count>=1;connected+=m.connected;balanced+=b;candidate+=c;symmetric+=s;out<<i<<','<<m.black_pixels<<','<<m.connected_components<<','<<(m.connected?1:0)<<','<<m.edge_count<<','<<m.transitions<<','<<m.signature<<','<<(b?1:0)<<','<<(c?1:0)<<'\n';}
 std::cout<<"samples="<<n<<"\nconnected="<<connected<<"\nbalanced_density="<<balanced<<"\ninterpretability_candidate="<<candidate<<"\nconnected_nonempty="<<symmetric<<"\n";
 return 0;
}

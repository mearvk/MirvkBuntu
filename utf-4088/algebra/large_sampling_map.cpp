#include "glyph8x12.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
std::uint64_t mix(std::uint64_t x){x^=x>>30;x*=0xbf58476d1ce4e5b9ULL;x^=x>>27;x*=0x94d049bb133111ebULL;return x^(x>>31);}
std::array<std::uint32_t,3> sample96(std::uint64_t& s){std::array<std::uint32_t,3>a{};for(auto&x:a){s=mix(s);x=static_cast<std::uint32_t>(s);}return a;}
utf4088::Glyph8x12 glyph(const std::array<std::uint32_t,3>&a){utf4088::Glyph8x12 g{};for(int y=0;y<12;++y){int w=y/4,sh=(y%4)*8;g.rows[y]=static_cast<std::uint8_t>((a[w]>>sh)&255U);}return g;}
}
int main(int argc,char**argv){
 const std::uint64_t n=argc>1?std::stoull(argv[1]):100000000ULL;const std::string outname=argc>2?argv[2]:"utf4088-large-sampling-map.csv";
 std::uint64_t state=0x40880812ULL,connected=0,balanced=0,candidate=0;std::vector<std::uint64_t> density(97,0),component(97,0),candidate_density(97,0);
 std::ofstream out(outname);if(!out){std::cerr<<"cannot open output\n";return 2;}out<<"sample,black_pixels,components,connected,edges,transitions,signature,candidate\n";
 for(std::uint64_t i=0;i<n;++i){auto m=utf4088::analyze_glyph(glyph(sample96(state)));++density[m.black_pixels];component[m.connected_components]++;if(m.connected)connected++;const bool b=m.black_pixels>=8&&m.black_pixels<=40; if(b)balanced++;const bool c=m.connected&&b&&m.transitions>=8&&m.transitions<=90&&m.edge_count>0;if(c){candidate++;candidate_density[m.black_pixels]++;}out<<i<<','<<m.black_pixels<<','<<m.connected_components<<','<<(m.connected?1:0)<<','<<m.edge_count<<','<<m.transitions<<','<<m.signature<<','<<(c?1:0)<<'\n';}
 std::ofstream map("utf4088-large-sampling-map-summary.csv");map<<"black_pixels,count,candidate_count\n";for(int i=0;i<=96;++i)if(density[i]||candidate_density[i])map<<i<<','<<density[i]<<','<<candidate_density[i]<<'\n';
 std::cout<<"samples="<<n<<"\nconnected="<<connected<<"\nconnected_rate="<<static_cast<long double>(connected)/n<<"\nbalanced_density="<<balanced<<"\nbalanced_rate="<<static_cast<long double>(balanced)/n<<"\ninterpretability_candidates="<<candidate<<"\ncandidate_rate="<<static_cast<long double>(candidate)/n<<"\n";
 return 0;
}

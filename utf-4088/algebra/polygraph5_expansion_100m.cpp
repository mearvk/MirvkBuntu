#include <cstdint>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

static inline std::uint64_t mix(std::uint64_t x){x^=x>>30;x*=0xbf58476d1ce4e5b9ULL;x^=x>>27;x*=0x94d049bb133111ebULL;return x^(x>>31);}
static inline double u01(std::uint64_t x){return (double)(x>>11)*(1.0/9007199254740992.0);}
static inline std::uint64_t glyph_hash(std::uint64_t a,std::uint64_t b){return mix(a^(b*0x9e3779b97f4a7c15ULL));}
int main(){
 const std::uint64_t N=100000000ULL; std::uint64_t s=0x40880812ULL; std::uint64_t accepted=0,hist[16]={};
 for(std::uint64_t i=0;i<N;i++){s=mix(s);auto a=s;s=mix(s);auto b=s;auto h=glyph_hash(a,b);double d=std::abs(u01(h)*2.0-1.0);double f=std::exp(-5.0*d);int bin=std::min(15,(int)(f*16.0));hist[bin]++;if(f>=0.001)accepted++;}
 std::cout<<"iterations="<<N<<"\naccepted="<<accepted<<"\nrate="<<std::setprecision(17)<<(double)accepted/N<<"\nminimum_falloff="<<std::exp(-5.0)<<"\n";
 for(int i=0;i<16;i++)std::cout<<"falloff_bin["<<i<<"]="<<hist[i]<<"\n";
}

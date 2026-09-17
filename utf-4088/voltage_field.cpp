#include "voltage_field.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace utf4088 {

FieldState analyze_field(const std::vector<VoltageSample>& samples, std::size_t i) {
    FieldState s{};
    if (samples.empty() || i >= samples.size()) return s;
    const auto& c = samples[i];
    s.voltage = c.volts;
    const VoltageSample* l=nullptr; const VoltageSample* r=nullptr;
    const VoltageSample* d=nullptr; const VoltageSample* u=nullptr;
    double ld=std::numeric_limits<double>::infinity(), rd=ld, dd=ld, ud=ld;
    for (const auto& p : samples) {
        if (&p == &c) continue;
        const double dx=p.x-c.x, dy=p.y-c.y, q=dx*dx+dy*dy;
        if (std::abs(dy)<1e-9 && dx<0 && q<ld) {l=&p;ld=q;}
        if (std::abs(dy)<1e-9 && dx>0 && q<rd) {r=&p;rd=q;}
        if (std::abs(dx)<1e-9 && dy<0 && q<dd) {d=&p;dd=q;}
        if (std::abs(dx)<1e-9 && dy>0 && q<ud) {u=&p;ud=q;}
    }
    if(l&&r) s.dVdx=(r->volts-l->volts)/(r->x-l->x);
    else if(r) s.dVdx=(r->volts-c.volts)/(r->x-c.x);
    else if(l) s.dVdx=(c.volts-l->volts)/(c.x-l->x);
    if(d&&u) s.dVdy=(u->volts-d->volts)/(u->y-d->y);
    else if(u) s.dVdy=(u->volts-c.volts)/(u->y-c.y);
    else if(d) s.dVdy=(c.volts-d->volts)/(c.y-d->y);
    s.magnitude=std::hypot(s.dVdx,s.dVdy);
    s.direction=std::atan2(s.dVdy,s.dVdx);
    double mean=0, var=0;
    for(const auto& p:samples) mean+=p.volts;
    mean/=samples.size();
    for(const auto& p:samples){double z=p.volts-mean;var+=z*z;}
    var/=samples.size();
    s.uniformity=1.0/(1.0+std::sqrt(var));
    return s;
}

std::uint64_t field_to_input(const FieldState& s) {
    auto q=[](double x)->std::uint64_t {
        if(!std::isfinite(x)||x<=0) return 0;
        return static_cast<std::uint64_t>(std::min(x,1.0e9)*1000.0);
    };
    const auto v=q(std::abs(s.voltage))&0xFFFFFULL;
    const auto m=q(s.magnitude)&0xFFFFFULL;
    const auto u=q(s.uniformity)&0xFFFFFULL;
    const auto d=q(std::abs(s.direction))&0xFFFFFULL;
    return (v<<44)|(m<<24)|(u<<4)|(d&0xFULL);
}

} // namespace utf4088

#include "norm_cost.hpp"
#include <algorithm>
#include <cmath>
namespace utf4088 {
NormObservation evaluate_norming(double deviation, double elapsed_time,
                                 double opportunity_value, const NormingProfile& p) {
    const double d=std::max(0.0,deviation);
    const double t=std::max(0.0,elapsed_time);
    const double o=std::max(0.0,opportunity_value);
    const double scatter=d/(1.0+d);
    const double norm=p.baseline*scatter*(1.0+t);
    const double fine=p.executive*scatter*scatter;
    const double opportunity=o*scatter;
    return {scatter,norm,fine,opportunity};
}
}

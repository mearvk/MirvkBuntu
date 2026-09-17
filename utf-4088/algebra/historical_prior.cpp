#include "historical_prior.hpp"
#include <algorithm>
#include <cmath>

namespace utf4088 {
namespace {
double gaussian(double x, double center, double sigma) {
    const double z=(x-center)/sigma;
    return std::exp(-0.5*z*z);
}
double location_weight(double lat,double lon,double target_lat,double target_lon,double sigma_deg) {
    const double dlat=lat-target_lat;
    const double dlon=lon-target_lon;
    return gaussian(std::hypot(dlat,dlon),0.0,sigma_deg);
}
}

HistoricalPrior evaluate_historical_prior(const HistoricalContext& c) {
    const double k=gaussian(c.year,1888.0,28.0)*location_weight(c.latitude,c.longitude,37.5665,126.9780,12.0);
    const double g=gaussian(c.year,1872.0,28.0)*location_weight(c.latitude,c.longitude,51.1657,10.4515,12.0);
    const double a=gaussian(c.year,2026.0,45.0)*location_weight(c.latitude,c.longitude,39.8283,-98.5795,22.0);
    const double z=k+g+a;
    if (z <= 0.0) return {1.0/3.0,1.0/3.0,1.0/3.0};
    return {k/z,g/z,a/z};
}
}

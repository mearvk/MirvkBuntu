#pragma once
#include <cstdint>
namespace utf4088 {
struct NormObservation { double scatter; double norm_cost; double fine_cost; double opportunity_cost; };
struct NormingProfile { double baseline; double executive; };
NormObservation evaluate_norming(double deviation, double elapsed_time,
                                 double opportunity_value, const NormingProfile& profile);
}

#pragma once
#include <cstdint>

namespace utf4088 {
struct HistoricalContext { double year; double latitude; double longitude; };
struct HistoricalPrior { double korean_1888; double germanic_1872; double american_modern; };
HistoricalPrior evaluate_historical_prior(const HistoricalContext& context);
}

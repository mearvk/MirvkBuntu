#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>

int main() {
    constexpr long double target = 4000000000.0L;
    constexpr long double raw = 79228162514264337593543950336.0L; // 2^96
    constexpr long double connected_rate = 276.0L / 1000000.0L;
    const long double connected_estimate = raw * connected_rate;
    const long double margin = raw / target;

    std::cout << std::setprecision(20);
    std::cout << "target=" << target << '\n';
    std::cout << "raw_96bit_capacity=" << raw << '\n';
    std::cout << "capacity_margin=" << margin << '\n';
    std::cout << "observed_connected_rate=" << connected_rate << '\n';
    std::cout << "extrapolated_connected_population=" << connected_estimate << '\n';
    std::cout << "capacity_pass=" << (raw >= target ? "true" : "false") << '\n';
    std::cout << "connected_subset_extrapolated_pass=" << (connected_estimate >= target ? "true" : "false") << '\n';
    std::cout << "semantic_interpretation_proven=false\n";
    return (raw >= target && connected_estimate >= target) ? 0 : 1;
}

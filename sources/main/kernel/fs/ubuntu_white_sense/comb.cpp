// COMB C++ facade for the Ubuntu White Sense prototype.
#include <array>
#include <string_view>
#include <iostream>

namespace ubuntu_white_sense {
constexpr std::array<std::string_view, 19> ratings{
    "use", "age", "homo", "homotype", "use_2", "useage", "manage",
    "action", "lists", "calls", "actionsagainst", "same", "came", "come",
    "hold", "research", "archer-class", "master-manager-class", "imperial-calls"
};

bool valid_sense_count(unsigned count) noexcept { return count >= 1 && count <= 3; }

void print_schema(std::ostream& out) {
    out << "Ubuntu White Sense v1\n";
    for (std::size_t i = 0; i < ratings.size(); ++i)
        out << (i + 1) << ": " << ratings[i] << '\n';
}
} // namespace ubuntu_white_sense

int main() {
    ubuntu_white_sense::print_schema(std::cout);
    return 0;
}

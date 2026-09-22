#pragma once
#include <string>
#include <vector>
namespace mirvkbuntu::installer {
struct Options { bool dry_run{true}; bool execute{false}; bool force{false}; std::string target_disk; std::string prefix; std::string language{"en_US"}; std::string keyboard{"us"}; std::string timezone{"UTC"}; };
struct Check { std::string name; bool available{false}; std::string detail; };
std::vector<Check> platformChecks();
int run(const Options& options);
void printPlan(const Options& options);
}
#pragma once
#include <string>
#include <vector>
namespace mirvkbuntu::installer {
struct Disk { std::string path; std::string size; std::string model; bool removable{false}; };
std::vector<Disk> enumerateDisks();
bool validateTarget(const std::string& disk);
int prepareLinuxStorage(const std::string& disk, bool execute);
}
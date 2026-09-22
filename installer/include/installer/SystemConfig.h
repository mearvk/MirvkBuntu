#pragma once
#include <string>
namespace mirvkbuntu::installer {
struct SystemConfig { std::string language; std::string keyboard; std::string timezone; std::string hostname; };
SystemConfig detectSystemConfig();
int applyLinuxSystemConfig(const SystemConfig&, bool execute);
}
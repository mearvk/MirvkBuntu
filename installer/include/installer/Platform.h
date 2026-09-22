#pragma once
#include <string>
namespace mirvkbuntu::installer {
std::string platformName();
bool isAdministrator();
int installPlatformPayload(const std::string& source, const std::string& prefix, bool execute);
}
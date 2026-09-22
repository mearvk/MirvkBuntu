#pragma once
#include <string>
namespace mirvkbuntu::installer { enum class BootMode { UEFI, BIOS }; int installBootloader(const std::string& target,BootMode mode,bool execute); }
#pragma once
#include <string>
namespace mirvkbuntu::installer { struct PayloadSpec { std::string root; std::string kernel; std::string initramfs; std::string source; }; int installPayload(const PayloadSpec&,const std::string& mount_point,bool execute); }
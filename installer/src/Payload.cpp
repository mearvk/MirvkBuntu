#include "installer/Payload.h"
#include <filesystem>
namespace fs=std::filesystem;
namespace mirvkbuntu::installer { int installPayload(const PayloadSpec& p,const std::string& mount,bool execute){ if(mount.empty()) return 2; if(!execute) return 0; std::error_code ec; fs::create_directories(mount,ec); return ec?3:0; } }
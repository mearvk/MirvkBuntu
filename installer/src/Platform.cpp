#include "installer/Platform.h"
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#elif __unix__
#include <unistd.h>
#endif
namespace fs=std::filesystem;
namespace mirvkbuntu::installer {
std::string platformName() {
#ifdef _WIN32
 return "Windows 10+";
#elif __APPLE__
 return "macOS";
#elif __linux__
 return "Linux";
#else
 return "Unknown";
#endif
}
bool isAdministrator() {
#ifdef _WIN32
 return IsUserAnAdmin()!=0;
#elif __unix__
 return geteuid()==0;
#else
 return false;
#endif
}
int installPlatformPayload(const std::string& source,const std::string& prefix,bool execute) {
 if(source.empty()||prefix.empty()) return 2; if(!execute) return 0;
 std::error_code ec; fs::create_directories(prefix,ec); return ec?3:0;
}
}
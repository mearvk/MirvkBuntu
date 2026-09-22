#include "installer/Installer.h"
#include "installer/Platform.h"
#include "installer/Storage.h"
#include "installer/SystemConfig.h"
#include <iostream>
namespace mirvkbuntu::installer {
std::vector<Check> platformChecks() {
 std::vector<Check> r; bool admin=isAdministrator(); r.push_back({"administrator/root",admin,admin?"privileged":"non-privileged"});
#ifdef __linux__
 r.push_back({"lsblk",std::system("command -v lsblk >/dev/null 2>&1")==0,"disk discovery"});
 r.push_back({"mkfs",std::system("command -v mkfs >/dev/null 2>&1")==0,"filesystem tools"});
 r.push_back({"localectl",std::system("command -v localectl >/dev/null 2>&1")==0,"locale/keyboard"});
 r.push_back({"timedatectl",std::system("command -v timedatectl >/dev/null 2>&1")==0,"timezone/clock"});
#elif _WIN32
 r.push_back({"Windows storage backend",true,"Win32/PowerShell backend"});
 r.push_back({"Windows locale backend",true,"Win32 locale backend"});
#elif __APPLE__
 r.push_back({"diskutil",std::system("command -v diskutil >/dev/null 2>&1")==0,"disk management"});
 r.push_back({"system configuration",true,"macOS backend"});
#else
 r.push_back({"supported platform",false,"unsupported host"});
#endif
 return r;
}
void printPlan(const Options&o) {
 std::cout<<"MirvkBuntu installation plan\nPlatform: "<<platformName()<<"\nLanguage: "<<o.language<<"\nKeyboard: "<<o.keyboard<<"\nTime zone: "<<o.timezone<<"\n";
 if(!o.target_disk.empty()) std::cout<<"Target disk: "<<o.target_disk<<"\n";
 std::cout<<"Storage: GPT/UEFI partition planning, filesystem creation, mount, install, bootloader\nSystem: locale, keyboard, timezone, hostname, network, users, packages, kernel/initramfs\nSafety: "<<(o.execute?"EXECUTE":"DRY RUN")<<"\n";
}
int run(const Options&o) {
 printPlan(o); if(!o.execute) return 0;
 if(o.target_disk.empty()) { std::cerr<<"No target disk supplied.\n"; return 2; }
 if(!validateTarget(o.target_disk)) { std::cerr<<"Invalid target disk.\n"; return 3; }
 return prepareLinuxStorage(o.target_disk,true);
}
}
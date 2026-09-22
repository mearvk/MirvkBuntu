#include "installer/Storage.h"
#include <cstdio>
#include <array>
#include <memory>
#include <sstream>
namespace mirvkbuntu::installer {
static std::string command(const char* cmd) {
 std::array<char,256> b{}; std::string out; std::unique_ptr<FILE,decltype(&pclose)> p(popen(cmd,"r"),pclose);
 if(!p) return {}; while(fgets(b.data(),b.size(),p.get())) out+=b.data(); return out;
}
std::vector<Disk> enumerateDisks() {
 std::vector<Disk> result;
#ifdef __linux__
 std::istringstream in(command("lsblk -dn -o PATH,SIZE,MODEL,RM 2>/dev/null"));
 Disk d; while(in>>d.path>>d.size>>d.model>>d.removable) result.push_back(d);
#endif
 return result;
}
bool validateTarget(const std::string& disk) {
 if(disk.empty()||disk.size()>256) return false;
#ifdef __linux__
 return disk.rfind("/dev/",0)==0;
#else
 return true;
#endif
}
int prepareLinuxStorage(const std::string& disk,bool execute) {
 if(!validateTarget(disk)) return 2;
 (void)execute; return 0;
}
}
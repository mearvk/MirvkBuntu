#include "installer/SystemConfig.h"
#include <cstdlib>
#include <fstream>
namespace mirvkbuntu::installer {
SystemConfig detectSystemConfig() {
 SystemConfig c; const char* lang=std::getenv("LANG"); c.language=lang&&*lang?lang:"en_US.UTF-8";
 const char* tz=std::getenv("TZ"); c.timezone=tz&&*tz?tz:"UTC"; c.keyboard="us"; c.hostname="mirvkbuntu";
#ifdef __linux__
 std::ifstream h("/etc/hostname"); std::string x; if(h&&std::getline(h,x)&&!x.empty()) c.hostname=x;
#endif
 return c;
}
int applyLinuxSystemConfig(const SystemConfig& c,bool execute) {
#ifdef __linux__
 if(!execute) return 0;
 std::string cmd="localectl set-locale LANG="+c.language; int rc=std::system(cmd.c_str()); if(rc!=0) return rc;
 cmd="timedatectl set-timezone "+c.timezone; return std::system(cmd.c_str());
#else
 (void)c;(void)execute;return 0;
#endif
}
}
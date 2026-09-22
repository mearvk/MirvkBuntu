#include "installer/Locale.h"
#include <algorithm>
namespace mirvkbuntu::installer {
std::vector<std::string> supportedLanguages(){return {"en_US.UTF-8","en_GB.UTF-8","de_DE.UTF-8","es_ES.UTF-8","fr_FR.UTF-8","it_IT.UTF-8","ja_JP.UTF-8","ko_KR.UTF-8","pt_BR.UTF-8","zh_CN.UTF-8","zh_TW.UTF-8"};}
std::vector<std::string> supportedTimezones(){return {"UTC","America/New_York","America/Chicago","America/Denver","America/Los_Angeles","Europe/London","Europe/Paris","Asia/Tokyo","Asia/Seoul","Asia/Shanghai","Australia/Sydney"};}
bool validLanguage(const std::string&s){auto v=supportedLanguages();return std::find(v.begin(),v.end(),s)!=v.end();}
bool validTimezone(const std::string&s){auto v=supportedTimezones();return std::find(v.begin(),v.end(),s)!=v.end();}
}
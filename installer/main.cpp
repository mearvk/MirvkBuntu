#include "installer/Installer.h"
#include "installer/Platform.h"
#include "installer/Locale.h"
#include "installer/Storage.h"
#include <iostream>
#include <string>

#ifndef MIRVKBUNTU_VERSION
#define MIRVKBUNTU_VERSION "dev"
#endif

static void usage(const char* p) {
 std::cout<<"MirvkBuntu Installer "<<MIRVKBUNTU_VERSION<<"\n"
 <<"Usage: "<<p<<" [--plan] [--check] [--execute] [--confirm] [--target-disk PATH]\n"
 <<"       [--language LOCALE] [--keyboard LAYOUT] [--timezone IANA_ZONE]\n"
 <<"       [--prefix PATH] [--source PATH]\n";
}
int main(int argc,char** argv) {
 mirvkbuntu::installer::Options o;
 bool check=false, plan=false, confirm=false;
 for(int i=1;i<argc;i++){
  std::string a=argv[i];
  if(a=="--help"||a=="-h"){usage(argv[0]);return 0;}
  if(a=="--dry-run") {o.dry_run=true;o.execute=false;}
  else if(a=="--plan") plan=true;
  else if(a=="--check") check=true;
  else if(a=="--execute") {o.execute=true;o.dry_run=false;}
  else if(a=="--confirm") confirm=true;
  else if(a=="--force") o.force=true;
  else if((a=="--target-disk"||a=="--prefix"||a=="--source"||a=="--language"||a=="--keyboard"||a=="--timezone")&&i+1<argc){
   std::string v=argv[++i];
   if(a=="--target-disk")o.target_disk=v; else if(a=="--prefix")o.prefix=v; else if(a=="--source"){} else if(a=="--language")o.language=v; else if(a=="--keyboard")o.keyboard=v; else o.timezone=v;
  } else {std::cerr<<"Unknown or incomplete option: "<<a<<"\n";return 2;}
 }
 std::cout<<"MirvkBuntu Installer "<<MIRVKBUNTU_VERSION<<" — "<<mirvkbuntu::installer::platformName()<<"\n";
 if(check){
  for(const auto& c:mirvkbuntu::installer::platformChecks()) std::cout<<(c.available?"[OK] ":"[--] ")<<c.name<<" — "<<c.detail<<"\n";
  return 0;
 }
 if(plan||!o.execute){mirvkbuntu::installer::printPlan(o);return 0;}
 if(!confirm&&!o.force){std::cerr<<"Refusing disk-affecting execution without --confirm. Use --plan first.\n";return 5;}
 return mirvkbuntu::installer::run(o);
}

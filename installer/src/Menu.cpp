#include "installer/Menu.h"
#include "installer/Installer.h"
#include <iostream>
#include <limits>
#include <string>
namespace mirvkbuntu::installer {
static int readChoice(int max) {
 int n=0; std::cout<<"Select: "; if(!(std::cin>>n)){std::cin.clear();std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');return -1;} return n>=1&&n<=max?n:-1;
}
int runMenu() {
 while(true) {
  std::cout<<"\n=== MirvkBuntu Installer ===\n";
  std::cout<<"Platform: "<<platformName()<<"\n\n";
  std::cout<<"1. Hardware & system check\n";
  std::cout<<"2. Storage & partitions\n";
  std::cout<<"3. Language & keyboard\n";
  std::cout<<"4. Time zone & clock\n";
  std::cout<<"5. Network & hostname\n";
  std::cout<<"6. User accounts\n";
  std::cout<<"7. MirvkBuntu payload\n";
  std::cout<<"8. Bootloader\n";
  std::cout<<"9. Installation plan\n";
  std::cout<<"10. Install / Continue\n";
  std::cout<<"0. Exit\n";
  int c=readChoice(10);
  if(c==0) return 0;
  if(c<0){std::cout<<"Invalid selection.\n";continue;}
  switch(c) {
   case 1: for(const auto& x:platformChecks()) std::cout<<(x.available?"[OK] ":"[--] ")<<x.name<<" — "<<x.detail<<"\n"; break;
   case 2: std::cout<<"Storage manager: disk discovery, GPT/UEFI plan, filesystem and mount stages.\n"; break;
   case 3: std::cout<<"Language/keyboard manager: locale and keyboard selection.\n"; break;
   case 4: std::cout<<"Time manager: IANA timezone and clock configuration.\n"; break;
   case 5: std::cout<<"Network manager: hostname and network configuration.\n"; break;
   case 6: std::cout<<"Account manager: user, groups and administrator configuration.\n"; break;
   case 7: std::cout<<"Payload manager: MirvkBuntu root filesystem, kernel and initramfs.\n"; break;
   case 8: std::cout<<"Boot manager: UEFI/bootloader installation stage.\n"; break;
   case 9: { Options o; printPlan(o); break; }
   case 10: std::cout<<"Installation remains protected by explicit target selection and confirmation.\n"; break;
  }
 }
}
}
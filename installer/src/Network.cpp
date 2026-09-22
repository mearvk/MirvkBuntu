#include "installer/Network.h"
namespace mirvkbuntu::installer { int configureNetwork(const NetworkSpec& n,bool execute){ if(n.hostname.empty()) return 2; (void)execute; return 0; } }
#include "installer/Accounts.h"
#include <cctype>
namespace mirvkbuntu::installer { int createUser(const UserSpec& u,bool execute){ if(u.name.empty()||!std::isalpha(static_cast<unsigned char>(u.name[0]))) return 2; (void)execute; return 0; } }
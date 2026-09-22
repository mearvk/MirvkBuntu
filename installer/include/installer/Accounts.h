#pragma once
#include <string>
namespace mirvkbuntu::installer { struct UserSpec { std::string name; std::string full_name; bool administrator{false}; }; int createUser(const UserSpec&,bool execute); }
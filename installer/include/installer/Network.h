#pragma once
#include <string>
namespace mirvkbuntu::installer { struct NetworkSpec { std::string hostname{"mirvkbuntu"}; bool automatic{true}; }; int configureNetwork(const NetworkSpec&,bool execute); }
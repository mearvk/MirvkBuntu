#pragma once
#include <string>
#include <vector>
namespace mirvkbuntu::installer { std::vector<std::string> supportedLanguages(); std::vector<std::string> supportedTimezones(); bool validLanguage(const std::string&); bool validTimezone(const std::string&); }
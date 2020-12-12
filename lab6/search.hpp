#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <string>
#include <vector>

std::vector<unsigned int> PrefixFunction(const std::string & s);
std::vector<unsigned int> KMPWeak(const std::string & pattern, const std::string & text);

std::vector<unsigned int> ZFunction(const std::string & s);
std::vector<unsigned int> StrongPrefixFunction(const std::string & s);
std::vector<unsigned int> KMPStrong(const std::string & pattern, const std::string & text);

#endif /* SEARCH_HPP */
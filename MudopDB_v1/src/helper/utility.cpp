#include "helper/utility.hpp"
#include <algorithm>
#include <cctype>

void trim_end(std::string &s) {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
    s.pop_back();
  }
}

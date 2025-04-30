#pragma once
#include <string>

namespace waddup {
  class Hash {
  public:
    static std::string calculate(const std::string &filepath);
  };
}  // namespace waddup

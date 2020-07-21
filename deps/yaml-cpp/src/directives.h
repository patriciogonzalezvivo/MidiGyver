#pragma once

#include <string>
#include <map>

namespace YAML {
struct Version {
  bool isDefault;
  int major, minor;
};

struct Directives {
  Directives();

  const std::string TranslateTagHandle(const std::string& handle) const;

  Version version;
  std::map<std::string, std::string> tags;
};
}

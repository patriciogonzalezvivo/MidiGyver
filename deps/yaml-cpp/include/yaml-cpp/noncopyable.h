#pragma once

#include "yaml-cpp/dll.h"

namespace YAML {
// this is basically boost::noncopyable
class YAML_CPP_API noncopyable {
 protected:
  noncopyable() {}
  ~noncopyable() {}

 private:
  noncopyable(const noncopyable&);
  const noncopyable& operator=(const noncopyable&);
};
}

#include "yaml-cpp/null.h"

namespace YAML {
_Null Null;

bool IsNullString(const std::string& str) {
  // Match empty | ~ | null | Null | NULL
  switch (str.size()) {
  case 0:
      return true;
  case 1:
      return str[0] == '~';
  case 4:
      if (str[0] == 'n') {
          return (str[1] == 'u' &&
                  str[2] == 'l' &&
                  str[3] == 'l');
      } else if (str[0] == 'N') {
          return ((str[1] == 'u' &&
                   str[2] == 'l' &&
                   str[3] == 'l') ||
                  (str[1] == 'U' &&
                   str[2] == 'L' &&
                   str[3] == 'L'));
      }
  default:
      break;
  }
  return false;
}
}

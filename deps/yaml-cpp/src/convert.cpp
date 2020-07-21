#include <algorithm>

#include "yaml-cpp/node/convert.h"

namespace {
// we're not gonna mess with the mess that is all the isupper/etc. functions
bool IsLower(char ch) { return 'a' <= ch && ch <= 'z'; }
bool IsUpper(char ch) { return 'A' <= ch && ch <= 'Z'; }
char ToLower(char ch) { return IsUpper(ch) ? ch + 'a' - 'A' : ch; }

std::string tolower(const std::string& str) {
  std::string s(str);
  std::transform(s.begin(), s.end(), s.begin(), ToLower);
  return s;
}

template <typename T>
bool IsEntirely(const std::string& str, T func) {
  for (std::size_t i = 0; i < str.size(); i++)
    if (!func(str[i]))
      return false;

  return true;
}

// IsFlexibleCase
// . Returns true if 'str' is:
//   . UPPERCASE
//   . lowercase
//   . Capitalized
bool IsFlexibleCase(const std::string& str) {
  if (str.empty())
    return true;

  if (IsEntirely(str, IsLower))
    return true;

  bool firstcaps = IsUpper(str[0]);
  std::string rest = str.substr(1);
  return firstcaps && (IsEntirely(rest, IsLower) || IsEntirely(rest, IsUpper));
}
}

namespace YAML {
bool convert<bool>::decode(const Node& node, bool& rhs) {
  if (!node.IsScalar())
    return false;

  // Check that length matches possible values
  if (node.Scalar().size() != (sizeof("true") - 1) &&
      node.Scalar().size() != (sizeof("false") - 1)) {
      return false;
  }

  // Only allow capitalized all uppercase or lowercase
  if (!IsFlexibleCase(node.Scalar()))
    return false;

  // Check for true/false, TRUE/FALSE and True/False
  std::string value = tolower(node.Scalar());

  if (value == "true") {
      rhs = true;
      return true;
  }
  if (value == "false") {
      rhs = false;
      return true;
  }

  return false;
}
}

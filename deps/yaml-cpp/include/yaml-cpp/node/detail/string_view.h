#pragma once

#include <string>

namespace YAML {
namespace detail {

struct string_view {
  const char* str;
  std::size_t length;

  bool equals(const std::string& other) const {
    return length == other.length() &&
      other.compare(0, length, str) == 0;
  }
};


template<class T>
struct is_string_comparable : std::integral_constant<
  bool,
  std::is_same<std::string, typename std::decay<T>::type>::value ||
  std::is_same<char const *, typename std::decay<T>::type>::value ||
  std::is_same<char *, typename std::decay<T>::type>::value
  > {};
}
}

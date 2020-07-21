#pragma once

namespace YAML {
namespace detail {
struct unspecified_bool {
  struct NOT_ALLOWED;
  static void true_value(NOT_ALLOWED*) {}
};
typedef void (*unspecified_bool_type)(unspecified_bool::NOT_ALLOWED*);
}
}

#define YAML_CPP_OPERATOR_BOOL()                                            \
  operator YAML::detail::unspecified_bool_type() const {                    \
    return this->operator!() ? 0                                            \
                             : &YAML::detail::unspecified_bool::true_value; \
  }

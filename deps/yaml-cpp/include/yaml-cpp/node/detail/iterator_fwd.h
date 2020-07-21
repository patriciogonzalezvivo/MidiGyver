#pragma once

#include "yaml-cpp/dll.h"
#include <list>
#include <utility>
#include <vector>

namespace YAML {

namespace detail {
struct iterator_value;
template <typename V>
class iterator_base;
}

typedef detail::iterator_base<detail::iterator_value> iterator;
typedef detail::iterator_base<const detail::iterator_value> const_iterator;
}

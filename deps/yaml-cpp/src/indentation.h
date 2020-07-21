#pragma once

#include <iostream>
#include <cstddef>

#include "yaml-cpp/ostream_wrapper.h"

namespace YAML {
struct Indentation {
  Indentation(std::size_t n_) : n(n_) {}
  std::size_t n;
};

inline ostream_wrapper& operator<<(ostream_wrapper& out,
                                   const Indentation& indent) {
  for (std::size_t i = 0; i < indent.n; i++)
    out << ' ';
  return out;
}

struct IndentTo {
  IndentTo(std::size_t n_) : n(n_) {}
  std::size_t n;
};

inline ostream_wrapper& operator<<(ostream_wrapper& out,
                                   const IndentTo& indent) {
  while (out.col() < indent.n)
    out << ' ';
  return out;
}
}

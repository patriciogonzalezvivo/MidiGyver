#pragma once

#include <string>

namespace YAML {
struct Directives;
struct Token;

struct Tag {
  enum TYPE : char {
    VERBATIM,
    PRIMARY_HANDLE,
    SECONDARY_HANDLE,
    NAMED_HANDLE,
    NON_SPECIFIC
  };

  Tag(const Token& token);
  const std::string Translate(const Directives& directives);

  TYPE type;
  std::string handle, value;
};
}

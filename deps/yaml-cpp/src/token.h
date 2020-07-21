#pragma once

#include "yaml-cpp/mark.h"
#include <iostream>
#include <string>
#include <vector>
#include <memory>

namespace YAML {
const std::string TokenNames[] = {
    "DIRECTIVE", "DOC_START", "DOC_END", "BLOCK_SEQ_START", "BLOCK_MAP_START",
    "BLOCK_SEQ_END", "BLOCK_MAP_END", "BLOCK_ENTRY", "FLOW_SEQ_START",
    "FLOW_MAP_START", "FLOW_SEQ_END", "FLOW_MAP_END", "FLOW_MAP_COMPACT",
    "FLOW_ENTRY", "KEY", "VALUE", "ANCHOR", "ALIAS", "TAG", "SCALAR", "NON_PLAIN_SCALAR"};

struct Token {
  // enums
  enum STATUS : char { VALID, INVALID, UNVERIFIED };
  enum TYPE : char {
    NONE = 0,
    PLAIN_SCALAR = 1,
    NON_PLAIN_SCALAR,
    FLOW_SEQ_START,
    BLOCK_SEQ_START,
    FLOW_MAP_START,
    BLOCK_MAP_START,
    KEY,
    VALUE,
    DIRECTIVE,
    DOC_START,
    DOC_END,
    BLOCK_SEQ_END,
    BLOCK_MAP_END,
    BLOCK_ENTRY,
    FLOW_SEQ_END,
    FLOW_MAP_END,
    FLOW_MAP_COMPACT,
    FLOW_ENTRY,
    ANCHOR,
    ALIAS,
    TAG,
  };

  // data
  Token() {}

  Token(TYPE type_, Mark mark_)
      : type(type_), status(VALID), data(0), mark(mark_) {}

  Token(TYPE type_, Mark mark_, std::string&& value_)
      : type(type_), status(VALID), data(0), mark(mark_), value(std::move(value_)) {}

  friend std::ostream& operator<<(std::ostream& out, const Token& token) {
    out << TokenNames[token.type] << std::string(": ") << token.value;
    if (token.params) {
        for (auto& p : *token.params) {
            out << std::string(" ") << p;
        }
    }
    return out;
  }

  void clearParam() {
    if (params) { params->clear(); }
  }
  void pushParam(std::string param) {
    if (!params) {
      params = std::unique_ptr<std::vector<std::string>>(new std::vector<std::string>);
    }
    params->push_back(std::move(param));
  }

  TYPE type;
  STATUS status;
  char data;
  Mark mark;
  std::string value;
  std::unique_ptr<std::vector<std::string>> params;
};
}

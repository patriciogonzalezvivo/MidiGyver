#pragma once

#include <map>
#include <memory>
#include <string>

#include "yaml-cpp/anchor.h"
#include "yaml-cpp/noncopyable.h"

namespace YAML {
class CollectionStack;
class EventHandler;
class Node;
class Scanner;
struct Directives;
struct Mark;
struct Token;

class SingleDocParser : private noncopyable {
 public:
  SingleDocParser(Scanner& scanner, const Directives& directives);
  ~SingleDocParser();

  void HandleDocument(EventHandler& eventHandler);

 private:
  void HandleNode(EventHandler& eventHandler);
  Token::TYPE HandleNodeOpen(EventHandler& eventHandler);

  void HandleBlockSequence(EventHandler& eventHandler);
  void HandleFlowSequence(EventHandler& eventHandler);

  void HandleBlockMap(EventHandler& eventHandler);
  void HandleFlowMap(EventHandler& eventHandler);
  void HandleCompactMap(EventHandler& eventHandler);
  void HandleCompactMapWithNoKey(EventHandler& eventHandler);

  bool ParseProperties(std::string& tag, anchor_t& anchor);
  void ParseTag(std::string& tag);
  void ParseAnchor(anchor_t& anchor);

  anchor_t RegisterAnchor(const std::string& name);
  anchor_t LookupAnchor(const Mark& mark, const std::string& name) const;

 private:
  Scanner& m_scanner;
  const Directives& m_directives;
  std::unique_ptr<CollectionStack> m_pCollectionStack;

  typedef std::map<std::string, anchor_t> Anchors;
  Anchors m_anchors;

  anchor_t m_curAnchor;
};
}

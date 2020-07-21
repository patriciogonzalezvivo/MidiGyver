#pragma once

#include <vector>

#include "yaml-cpp/anchor.h"
#include "yaml-cpp/emitterstyle.h"
#include "yaml-cpp/eventhandler.h"
#include "yaml-cpp/node/detail/memory.h"

namespace YAML {
namespace detail {
class node;
}  // namespace detail
struct Mark;
}  // namespace YAML

namespace YAML {
class Node;

class NodeBuilder : public EventHandler {
 public:
  NodeBuilder();
  ~NodeBuilder() override;

  Node Root();

  void OnDocumentStart(const Mark& mark) override;
  void OnDocumentEnd() override;

  void OnNull(const Mark& mark, anchor_t anchor) override;
  void OnAlias(const Mark& mark, anchor_t anchor) override;
  void OnScalar(const Mark& mark, const std::string& tag,
                anchor_t anchor, std::string value) override;

  void OnSequenceStart(const Mark& mark, const std::string& tag,
                       anchor_t anchor, EmitterStyle style) override;
  void OnSequenceEnd() override;

  void OnMapStart(const Mark& mark, const std::string& tag,
                  anchor_t anchor, EmitterStyle style) override;
  void OnMapEnd() override;

 private:
  detail::node& Push(const Mark& mark, anchor_t anchor);
  void Push(detail::node& node);
  void Pop();
  void RegisterAnchor(anchor_t anchor, detail::node& node);

 private:
  detail::shared_memory m_pMemory;
  detail::node* m_pRoot;

  typedef std::vector<detail::node*> Nodes;
  Nodes m_stack;
  Nodes m_anchors;

  typedef std::pair<detail::node*, bool> PushedKey;
  std::vector<PushedKey> m_keys;
  std::size_t m_mapDepth;
};
}

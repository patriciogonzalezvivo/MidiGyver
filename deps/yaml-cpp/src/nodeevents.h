#pragma once

#include <map>
#include <vector>

#include "yaml-cpp/anchor.h"
#include "yaml-cpp/node/detail/memory.h"

namespace YAML {
namespace detail {
class node;
class node_data;
}  // namespace detail
}  // namespace YAML

namespace YAML {
class EventHandler;
class Node;

class NodeEvents {
 public:
  explicit NodeEvents(const Node& node);

  void Emit(EventHandler& handler);

 private:
  class AliasManager {
   public:
    AliasManager() : m_curAnchor(0) {}

    void RegisterReference(const detail::node& node);
    anchor_t LookupAnchor(const detail::node& node) const;

   private:
    anchor_t _CreateNewAnchor() { return ++m_curAnchor; }

   private:
    typedef std::map<const detail::node_data*, anchor_t> AnchorByIdentity;
    AnchorByIdentity m_anchorByIdentity;

    anchor_t m_curAnchor;
  };

  void Setup(const detail::node& node);
  void Emit(const detail::node& node, EventHandler& handler,
            AliasManager& am) const;
  bool IsAliased(const detail::node& node) const;

 private:
  detail::shared_memory m_pMemory;
  detail::node* m_root;

  typedef std::map<const detail::node_data*, int> RefCount;
  RefCount m_refCount;
};
}

#include "yaml-cpp/exceptions.h"
#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/impl.h"
#include "yaml-cpp/node/detail/node.h"
#include "yaml-cpp/node/detail/node_data.h"
#include "yaml-cpp/node/convert.h"
#include "yaml-cpp/node/detail/impl.h"
#include "yaml-cpp/nodebuilder.h"
#include "nodeevents.h"

namespace YAML {
Node Clone(const Node& node) {
  NodeEvents events(node);
  NodeBuilder builder;
  events.Emit(builder);
  return builder.Root();
}
void Node::ThrowInvalidNode() const {
  throw InvalidNode();
}

const Node Node::get(const detail::string_view& key) const {
  ThrowOnInvalid();
  EnsureNodeExists();

  detail::node* value = static_cast<const detail::node&>(*m_pNode).get(key, m_pMemory);

  if (!value) {
    return Node(ZombieNode);
  }
  return Node(*value, m_pMemory);
}

Node Node::get(const detail::string_view& key) {
  ThrowOnInvalid();
  EnsureNodeExists();

  detail::node& value = m_pNode->get(key, m_pMemory);
  return Node(value, m_pMemory);
}

namespace detail {
void node::mark_defined() {
  if (is_defined())
    return;

  m_pRef->mark_defined();

  if (m_dependencies) {
    for (auto& it : *m_dependencies) {
      it->mark_defined();
    }
    m_dependencies.reset();
  }
}

void node::add_dependency(node& rhs) {
  if (is_defined())
    rhs.mark_defined();
  else {
    if (!m_dependencies) {
      m_dependencies = std::unique_ptr<nodes>(new nodes);
    }
    m_dependencies->insert(&rhs);
  }
}
}
}

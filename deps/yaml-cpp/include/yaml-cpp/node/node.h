#pragma once

#include <stdexcept>

#include "yaml-cpp/dll.h"
#include "yaml-cpp/emitterstyle.h"
#include "yaml-cpp/mark.h"
#include "yaml-cpp/node/detail/bool_type.h"
#include "yaml-cpp/node/detail/iterator_fwd.h"
#include "yaml-cpp/node/detail/string_view.h"
#include "yaml-cpp/node/detail/memory.h"
#include "yaml-cpp/node/type.h"

namespace YAML {
namespace detail {
class node;
class node_data;
struct iterator_value;
}  // namespace detail
}  // namespace YAML

namespace YAML {
class YAML_CPP_API Node {
 public:
  friend class NodeBuilder;
  friend class NodeEvents;
  friend struct detail::iterator_value;
  friend class detail::node;
  friend class detail::node_data;
  template <typename>
  friend class detail::iterator_base;
  template <typename T, typename S>
  friend struct as_if;
  template <typename T>
  friend struct convert;

  typedef YAML::iterator iterator;
  typedef YAML::const_iterator const_iterator;

  Node();
  ~Node();
  Node(const Node& rhs);
  Node(Node&& rhs);
  explicit Node(NodeType type);
  template <typename T>
  explicit Node(const T& rhs);

  YAML::Mark Mark() const;
  NodeType Type() const;
  bool IsDefined() const;
  bool IsNull() const { return Type() == NodeType::Null; }
  bool IsScalar() const { return Type() == NodeType::Scalar; }
  bool IsSequence() const { return Type() == NodeType::Sequence; }
  bool IsMap() const { return Type() == NodeType::Map; }

  // bool conversions
  YAML_CPP_OPERATOR_BOOL()
  bool operator!() const { return !IsDefined(); }

  // access
  template <typename T>
  T as() const;
  template <typename T, typename S>
  T as(const S& fallback) const;
  const std::string& Scalar() const;

  const std::string& Tag() const;
  void SetTag(const std::string& tag);

  // style
  // WARNING: This API might change in future releases.
  EmitterStyle Style() const;
  void SetStyle(EmitterStyle style);

  // assignment
  bool is(const Node& rhs) const;
  template <typename T>
  Node& operator=(const T& rhs);
  Node& operator=(const Node& rhs);

  // Reset Node to another Node (or create new Node)
  void reset(const Node& rhs = Node());

  // Set Node to undefined
  void clear();

  // size/iterator
  std::size_t size() const;

  const_iterator begin() const;
  iterator begin();

  const_iterator end() const;
  iterator end();

  // sequence
  template <typename T>
  void push_back(const T& rhs);
  void push_back(const Node& rhs);

  // indexing
  template <typename Key>
  const Node operator[](const Key& key) const;

  template <typename Key>
  Node operator[](const Key& key);

  template <typename Key>
  bool remove(const Key& key);

  const Node operator[](const Node& key) const;
  Node operator[](const Node& key);
  bool remove(const Node& key);

  // map
  template <typename Key, typename Value>
  void force_insert(const Key& key, const Value& value);

 private:
  enum Zombie { ZombieNode };
  explicit Node(Zombie);
  explicit Node(detail::node& node, detail::shared_memory pMemory);

  explicit Node(const detail::iterator_value& rhs, detail::shared_memory memory);

  template <typename T>
  inline Node(const T& rhs, detail::shared_memory memory);

  void EnsureNodeExists() const;

  template <typename T>
  void Assign(const T& rhs);
  void Assign(const char* rhs);
  void Assign(char* rhs);

  void AssignNode(const Node& rhs);

 private:
  mutable detail::shared_memory m_pMemory;
  mutable detail::node* m_pNode;

  void ThrowOnInvalid() const;
  void ThrowInvalidNode() const;
  bool isValid() const { return m_pMemory != nullptr; }

  void mergeMemory(const Node& rhs) const;
  detail::node& node() {
    return *m_pNode;
  }

  void set(detail::node& node, detail::shared_memory& pMemory) {
    m_pNode = &node;
    if (m_pMemory != pMemory) { m_pMemory = pMemory; }
  }

  template <typename Key>
  const Node get(const Key& key) const;
  const Node get(const detail::string_view& key) const;

  template <typename Key>
  Node get(const Key& key);
  Node get(const detail::string_view& key);
};

YAML_CPP_API bool operator==(const Node& lhs, const Node& rhs);

YAML_CPP_API Node Clone(const Node& node);

template <typename T>
struct convert;
}

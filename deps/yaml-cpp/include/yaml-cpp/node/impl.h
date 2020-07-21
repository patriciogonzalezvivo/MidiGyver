#pragma once

#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/iterator.h"
#include "yaml-cpp/node/detail/memory.h"
#include "yaml-cpp/node/detail/node.h"
#include "yaml-cpp/node/detail/string_view.h"
#include "yaml-cpp/exceptions.h"
#include <string>
#include <cstring>

namespace YAML {

inline Node::Node() :
    m_pMemory(new detail::memory_ref),
    m_pNode(nullptr) {
}

template <typename T>
inline Node::Node(const T& rhs)
    : m_pMemory(new detail::memory_ref),
      m_pNode(nullptr) {
    Assign(rhs);
}

inline Node::Node(NodeType type)
    : m_pMemory(new detail::memory_ref),
      m_pNode(&(m_pMemory->create_node())) {
  m_pNode->set_type(type);
}


inline Node::Node(const Node& rhs)
    : m_pMemory(rhs.m_pMemory),
      m_pNode(rhs.m_pNode) {
}

// Use by push_back(), operator=()
template <typename T>
inline Node::Node(const T& rhs, detail::shared_memory memory)
    : m_pMemory(memory),
      m_pNode(nullptr) {
  Assign(rhs);
}

inline Node::Node(const detail::iterator_value& rhs, detail::shared_memory memory)
    : m_pMemory(memory), m_pNode(rhs.m_pNode) {

    if (m_pMemory != rhs.m_pMemory) {
      m_pMemory->merge(*rhs.m_pMemory);
      rhs.m_pMemory = m_pMemory;
  }
}

inline Node::Node(Zombie) : m_pMemory(nullptr), m_pNode(nullptr) {}

inline Node::Node(Node&& rhs)
    : m_pMemory(std::move(rhs.m_pMemory)), m_pNode(rhs.m_pNode) {
  rhs.m_pNode = nullptr;
}

inline Node::Node(detail::node& node, detail::shared_memory pMemory)
    : m_pMemory(pMemory), m_pNode(&node) {}

inline Node::~Node() {}

inline void Node::ThrowOnInvalid() const {
  if (__builtin_expect(!m_pMemory, 0)) { ThrowInvalidNode(); };
}

inline void Node::EnsureNodeExists() const {
  ThrowOnInvalid();

  if (!m_pNode) {
    m_pNode = &m_pMemory->create_node();
    m_pNode->set_null();
  }
}

inline bool Node::IsDefined() const {
  if (isValid()) {
      // FIXME should be:
      //return m_pNode ? m_pNode->is_defined() : false;
      return m_pNode ? m_pNode->is_defined() : true;
  }
  return false;
}

inline Mark Node::Mark() const {
  ThrowOnInvalid();

  return m_pNode ? m_pNode->mark() : Mark::null_mark();
}

inline NodeType Node::Type() const {
  ThrowOnInvalid();
  // FIXME should be:
  //return m_pNode ? m_pNode->type() : NodeType::Undefined;
  return m_pNode ? m_pNode->type() : NodeType::Null;
}

// access

// template helpers
template <typename T, typename S>
struct as_if {
  explicit as_if(const Node& node_) : node(node_) {}
  const Node& node;

  T operator()(const S& fallback) const {
    if (!node.m_pNode)
      return fallback;

    T t;
    if (convert<T>::decode(node, t))
      return t;
    return fallback;
  }
};

template <typename S>
struct as_if<std::string, S> {
  explicit as_if(const Node& node_) : node(node_) {}
  const Node& node;

  std::string operator()(const S& fallback) const {
    if (node.Type() != NodeType::Scalar)
      return fallback;
    return node.Scalar();
  }
};

template <typename T>
struct as_if<T, void> {
  explicit as_if(const Node& node_) : node(node_) {}
  const Node& node;

  T operator()() const {
    if (!node.m_pNode)
      throw TypedBadConversion<T>(node.Mark());

    T t;
    if (convert<T>::decode(node, t))
      return t;
    throw TypedBadConversion<T>(node.Mark());
  }
};

template <>
struct as_if<std::string, void> {
  explicit as_if(const Node& node_) : node(node_) {}
  const Node& node;

  std::string operator()() const {
    if (node.Type() != NodeType::Scalar)
      throw TypedBadConversion<std::string>(node.Mark());
    return node.Scalar();
  }
};

// access functions
template <typename T>
inline T Node::as() const {
  ThrowOnInvalid();
  return as_if<T, void>(*this)();
}

template <typename T, typename S>
inline T Node::as(const S& fallback) const {
  if (isValid()) {
    return as_if<T, S>(*this)(fallback);
  }
  return fallback;
}

inline const std::string& Node::Scalar() const {
  ThrowOnInvalid();
  return m_pNode ? m_pNode->scalar() : detail::node_data::empty_scalar;
}

inline const std::string& Node::Tag() const {
  ThrowOnInvalid();
  return m_pNode ? m_pNode->tag() : detail::node_data::empty_scalar;
}

inline void Node::SetTag(const std::string& tag) {
  ThrowOnInvalid();
  EnsureNodeExists();
  m_pNode->set_tag(tag);
}

inline EmitterStyle Node::Style() const {
  ThrowOnInvalid();
  return m_pNode ? m_pNode->style() : EmitterStyle::Default;
}

inline void Node::SetStyle(EmitterStyle style) {
  ThrowOnInvalid();
  EnsureNodeExists();
  m_pNode->set_style(style);
}

// assignment
inline bool Node::is(const Node& rhs) const {
  ThrowOnInvalid();
  rhs.ThrowOnInvalid();

  if (!m_pNode || !rhs.m_pNode)
    return false;
  return m_pNode->is(*rhs.m_pNode);
}


inline void Node::reset(const YAML::Node& rhs) {
  ThrowOnInvalid();
  rhs.ThrowOnInvalid();

  m_pMemory = rhs.m_pMemory;
  m_pNode = rhs.m_pNode;
}

inline void Node::clear() {
  ThrowOnInvalid();

  if (m_pNode) {
    m_pNode->set_type(NodeType::Null);
  } else {
    EnsureNodeExists();
  }
}

template <typename T>
inline void Node::Assign(const T& rhs) {
  EnsureNodeExists();
  convert<T>::encode(rhs, *this);
}

template <>
inline void Node::Assign(const std::string& rhs) {
  EnsureNodeExists();
  m_pNode->set_scalar(rhs);
}

inline void Node::Assign(const char* rhs) {
  EnsureNodeExists();
  m_pNode->set_scalar(rhs);
}

inline void Node::Assign(char* rhs) {
  EnsureNodeExists();
  m_pNode->set_scalar(rhs);
}

template <typename T>
inline Node& Node::operator=(const T& rhs) {
  ThrowOnInvalid();
  Assign(rhs);
  return *this;
}

inline Node& Node::operator=(const Node& rhs) {
  ThrowOnInvalid();
  rhs.ThrowOnInvalid();

  if (is(rhs)) {
    return *this;
  }
  AssignNode(rhs);
  return *this;
}

inline void Node::AssignNode(const Node& rhs) {

  rhs.EnsureNodeExists();

  if (!m_pNode) {
    m_pNode = rhs.m_pNode;
    m_pMemory = rhs.m_pMemory;
    return;
  }

  // Update any Node aliasing m_pNode
  // (NodeTest.SimpleAlias)
  m_pNode->set_ref(*rhs.m_pNode);

  // All aliasing Nodes will have the same shared_memory,
  // so any nodes referenced by rhs will be added to their
  // shared_memory as well
  // (NodeTest.ChildNodesAliveAfterOwnerNodeExitsScope)
  mergeMemory(rhs);
  m_pNode = rhs.m_pNode;
}

inline void Node::mergeMemory(const Node& rhs) const {
  if (m_pMemory != rhs.m_pMemory) {
      m_pMemory->merge(*rhs.m_pMemory);
      rhs.m_pMemory = m_pMemory;
  }
}

// size/iterator
inline std::size_t Node::size() const {
  ThrowOnInvalid();

  return m_pNode ? m_pNode->size() : 0;
}

inline const_iterator Node::begin() const {
  if (isValid() && m_pNode)
    return const_iterator(m_pNode->begin(), m_pMemory);

  return const_iterator();
}

inline iterator Node::begin() {
  if (isValid() && m_pNode)
    return iterator(m_pNode->begin(), m_pMemory);

  return iterator();
}

inline const_iterator Node::end() const {
  if (isValid() && m_pNode)
    return const_iterator(m_pNode->end(), m_pMemory);

  return const_iterator();
}

inline iterator Node::end() {
  if (isValid() && m_pNode)
    return iterator(m_pNode->end(), m_pMemory);

  return iterator();
}

// sequence
template <typename T>
inline void Node::push_back(const T& rhs) {
  ThrowOnInvalid();

  Node value(rhs, m_pMemory);

  EnsureNodeExists();
  m_pNode->push_back(*value.m_pNode, m_pMemory);
}

inline void Node::push_back(const Node& rhs) {
  ThrowOnInvalid();
  rhs.ThrowOnInvalid();

  EnsureNodeExists();
  rhs.EnsureNodeExists();

  m_pNode->push_back(*rhs.m_pNode, m_pMemory);
  mergeMemory(rhs);
}

// helpers for indexing
namespace detail {
template <typename T>
struct to_value_t {
  explicit to_value_t(const T& t_) : t(t_) {}
  const T& t;
  typedef const T& return_type;

  const T& operator()() const { return t; }
};

template <>
struct to_value_t<std::string> {
  explicit to_value_t(const std::string& t_) : t(t_) {}
  const std::string& t;
  typedef detail::string_view return_type;

  const detail::string_view operator()() const {
    return { t.data(), t.length() } ;
  }
};

template <>
struct to_value_t<const char*> {
  explicit to_value_t(const char* t_) : t(t_) {}
  const char* t;
  typedef detail::string_view return_type;

  const detail::string_view operator()() const {
    return { t, strlen(t) } ;
  }
};

template <>
struct to_value_t<char*> {
  explicit to_value_t(char* t_) : t(t_) {}
  const char* t;
  typedef detail::string_view return_type;

  const detail::string_view operator()() const {
    return { t, strlen(t) } ;
  }
};

template <std::size_t N>
struct to_value_t<char[N]> {
  explicit to_value_t(const char* t_) : t(t_) {}
  const char* t;
  typedef detail::string_view return_type;
  const detail::string_view operator()() const {
        return { t, N-1 } ;
  }
};

// converts C-strings to std::strings so they can be copied
template <typename T>
inline typename to_value_t<T>::return_type to_value(const T& t) {
  return to_value_t<T>(t)();
}
}

// indexing
template <typename Key>
inline const Node Node::operator[](const Key& key) const {
  return get(detail::to_value(key));
}

template <typename Key>
inline Node Node::operator[](const Key& key) {
  return get(detail::to_value(key));
}

template <typename Key>
const Node Node::get(const Key& key) const {
  ThrowOnInvalid();
  EnsureNodeExists();

  detail::node* value = static_cast<const detail::node&>(*m_pNode).get(key, m_pMemory);

  if (!value) {
    return Node(ZombieNode);
  }
  return Node(*value, m_pMemory);
}

template <typename Key>
Node Node::get(const Key& key) {
  ThrowOnInvalid();
  EnsureNodeExists();

  detail::node& value = m_pNode->get(key, m_pMemory);
  return Node(value, m_pMemory);
}

template <typename Key>
inline bool Node::remove(const Key& key) {
  ThrowOnInvalid();
  EnsureNodeExists();
  return m_pNode->remove(detail::to_value(key), m_pMemory);
}

inline const Node Node::operator[](const Node& key) const {

  ThrowOnInvalid();
  key.ThrowOnInvalid();

  EnsureNodeExists();
  key.EnsureNodeExists();
  mergeMemory(key);

  detail::node* value =
    static_cast<const detail::node&>(*m_pNode).get(*key.m_pNode, m_pMemory);
  if (!value) {
    return Node(ZombieNode);
  }
  return Node(*value, m_pMemory);
}

inline Node Node::operator[](const Node& key) {
  ThrowOnInvalid();
  key.ThrowOnInvalid();

  EnsureNodeExists();
  key.EnsureNodeExists();
  mergeMemory(key);

  detail::node& value = m_pNode->get(*key.m_pNode, m_pMemory);
  return Node(value, m_pMemory);
}

inline bool Node::remove(const Node& key) {
  ThrowOnInvalid();
  key.ThrowOnInvalid();

  EnsureNodeExists();
  key.EnsureNodeExists();
  return m_pNode->remove(*key.m_pNode, m_pMemory);
}

// map
template <typename Key, typename Value>
inline void Node::force_insert(const Key& key, const Value& value) {
  ThrowOnInvalid();
  EnsureNodeExists();
  m_pNode->force_insert(detail::to_value(key),
                        detail::to_value(value),
                        m_pMemory);
}

// free functions
inline bool operator==(const Node& lhs, const Node& rhs) { return lhs.is(rhs); }
}

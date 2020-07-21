#pragma once

#include "yaml-cpp/emitterstyle.h"
#include "yaml-cpp/dll.h"
#include "yaml-cpp/node/type.h"
#include "yaml-cpp/node/ptr.h"
#include "yaml-cpp/node/detail/node_data.h"
#include "yaml-cpp/node/detail/memory.h"
#include <set>

namespace YAML {
namespace detail {
class node {

 using node_data_ref = ref_holder<node_data, false>;

 public:
  node() : m_pRef(nullptr) {}
  void set_data(node_data *data) { m_pRef = data; }

  __attribute__((noinline))
  ~node() {}

  node(const node&) = delete;
  // required for bucket reserve
  node(node&&) = default;
  node& operator=(const node&) = delete;
  node& operator=(node&&) = default;

  bool is(const node& rhs) const { return m_pRef == rhs.m_pRef; }
  const node_data* ref() const { return m_pRef.get(); }

  bool is_defined() const { return m_pRef->is_defined(); }
  const Mark& mark() const { return m_pRef->mark(); }
  NodeType type() const { return m_pRef->type(); }

  const std::string& scalar() const { return static_cast<const node_data&>(*m_pRef).scalar(); }
  const std::string& tag() const { return m_pRef->tag(); }
  EmitterStyle style() const { return m_pRef->style(); }

  template <typename T>
  bool equals(const T& rhs, shared_memory& pMemory);
  bool equals(const char* rhs, shared_memory& pMemory);

  // set shared data
  void set_ref(const node& rhs) {
    bool defined = rhs.is_defined();
    m_pRef = rhs.m_pRef;
    if (defined) {
      if (!is_defined()) mark_defined();
    }
  }

  void set_mark(const Mark& mark) { m_pRef->set_mark(mark); }

  void set_type(NodeType type) {
    if (type != NodeType::Undefined)
      mark_defined();
    m_pRef->set_type(type);
  }
  void set_null() {
    if (!is_defined()) mark_defined();
    m_pRef->set_null();
  }
  void set_scalar(const std::string& scalar) {
    if (!is_defined()) mark_defined();
    m_pRef->set_scalar(scalar);
  }
  void set_scalar(std::string&& scalar) {
    if (!is_defined()) mark_defined();
    m_pRef->set_scalar(std::move(scalar));
  }
  void set_tag(const std::string& tag) {
    if (!is_defined()) mark_defined();
    m_pRef->set_tag(tag);
  }

  // style
  void set_style(EmitterStyle style) {
    if (!is_defined()) mark_defined();
    m_pRef->set_style(style);
  }

  // size/iterator
  std::size_t size() const { return m_pRef->size(); }

  const_node_iterator begin() const {
    return static_cast<const node_data&>(*m_pRef).begin();
  }
  node_iterator begin() { return m_pRef->begin(); }

  const_node_iterator end() const {
    return static_cast<const node_data&>(*m_pRef).end();
  }
  node_iterator end() { return m_pRef->end(); }

  // sequence
  void push_back(node& input, shared_memory& pMemory) {
    m_pRef->push_back(input, pMemory);
    input.add_dependency(*this);
  }
  void insert(node& key, node& value, shared_memory& pMemory) {
    m_pRef->insert(key, value, pMemory);
    key.add_dependency(*this);
    value.add_dependency(*this);
  }

  // indexing
  template <typename Key>
  node* get(const Key& key, shared_memory& pMemory) const {
    // NOTE: this returns a non-const node so that the top-level Node can wrap
    // it, and returns a pointer so that it can be NULL (if there is no such
    // key).
    return static_cast<const node_data&>(*m_pRef).get(key, pMemory);
  }
  template <typename Key>
  node& get(const Key& key, shared_memory& pMemory) {
    node& value = m_pRef->get(key, pMemory);
    value.add_dependency(*this);
    return value;
  }

  template <typename Key>
  bool remove(const Key& key, shared_memory& pMemory) {
    return m_pRef->remove(key, pMemory);
  }

  node* get(node& key, shared_memory& pMemory) const {
    // NOTE: this returns a non-const node so that the top-level Node can wrap
    // it, and returns a pointer so that it can be NULL (if there is no such
    // key).
    return static_cast<const node_data&>(*m_pRef).get(key, pMemory);
  }
  node& get(node& key, shared_memory& pMemory) {
    node& value = m_pRef->get(key, pMemory);
    if (!key.is_defined() || !value.is_defined()) {
      key.add_dependency(*this);
      value.add_dependency(*this);
    } else {
      mark_defined();
    }
    return value;
  }
  bool remove(node& key, shared_memory& pMemory) {
    return m_pRef->remove(key, pMemory);
  }

  // map
  template <typename Key, typename Value>
  void force_insert(const Key& key, const Value& value, shared_memory& pMemory) {
    m_pRef->force_insert(key, value, pMemory);
  }

 private:
  void mark_defined();

  void add_dependency(node& rhs);

  mutable node_data_ref m_pRef;
  typedef std::set<node*> nodes;
  std::unique_ptr<nodes> m_dependencies;
};
}
}

#pragma once

#include <forward_list>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <cassert>

#include "yaml-cpp/dll.h"
#include "yaml-cpp/node/detail/node_iterator.h"
#include "yaml-cpp/node/detail/string_view.h"
#include "yaml-cpp/node/iterator.h"
#include "yaml-cpp/node/ptr.h"
#include "yaml-cpp/node/type.h"

namespace YAML {
namespace detail {
class node;

}  // namespace detail
}  // namespace YAML

namespace YAML {
namespace detail {

template <std::size_t A, std::size_t... C>
struct static_max;

template <std::size_t N>
struct static_max<N> {
  static const std::size_t value = N;
};
template <std::size_t A, std::size_t B, std::size_t... C>
struct static_max<A, B, C...> {
  static const std::size_t value = A >= B ?
    static_max<A, C...>::value :
    static_max<B, C...>::value;
};

class YAML_CPP_API node_data : public ref_counted {
 public:
  node_data();
  ~node_data();
  node_data(const node_data&) = delete;
  node_data& operator=(const node_data&) = delete;
  node_data& operator=(node_data&&) = default;

  void mark_defined();
  void set_type(NodeType type);
  void set_tag(const std::string& tag);
  void set_null();
  void set_scalar(const std::string& scalar);
  void set_scalar(std::string&& scalar);

  void set_mark(const Mark& mark) { m_mark = mark; }
  void set_style(EmitterStyle style) { m_style = style; }

  bool is_defined() const { return m_type != NodeType::Undefined; }
  const Mark& mark() const { return m_mark; }
  NodeType type() const {
    return m_type;
  }
  const std::string& scalar() const {
    if (m_type == NodeType::Scalar)
      return *reinterpret_cast<const std::string*>(&m_data);

    return empty_scalar;
  }
  const std::string& tag() const { return m_tag ? *m_tag : tag_none; }

  EmitterStyle style() const { return m_style; }

  // size/iterator
  std::size_t size() const;

  const_node_iterator begin() const;
  node_iterator begin();

  const_node_iterator end() const;
  node_iterator end();

  // sequence
  void push_back(node& node, shared_memory& pMemory);
  void insert(node& key, node& value, shared_memory& pMemory);

  // indexing
  template <typename Key,
            typename std::enable_if<std::is_same<Key, detail::string_view>::value, int>::type = 1>
    //typename detail::is_string_view<Key>>
  node* get(const Key& key, shared_memory& pMemory) const;

  template <typename Key,
            typename std::enable_if<!std::is_same<Key, detail::string_view>::value, int>::type = 0>
  node* get(const Key& key, shared_memory& pMemory) const;

  template <typename Key,
            typename std::enable_if<std::is_same<Key, detail::string_view>::value, int>::type = 1>
  node& get(const Key& key, shared_memory& pMemory);

  template <typename Key,
            typename std::enable_if<!std::is_same<Key, detail::string_view>::value, int>::type = 0>
  node& get(const Key& key, shared_memory& pMemory);

  template <typename Key,
            typename std::enable_if<std::is_same<Key, detail::string_view>::value, int>::type = 1>
  bool remove(const Key& key, shared_memory& pMemory);

  template <typename Key,
            typename std::enable_if<!std::is_same<Key, detail::string_view>::value, int>::type = 0>
  bool remove(const Key& key, shared_memory& pMemory);

  node* get(node& key, shared_memory& pMemory) const;
  node& get(node& key, shared_memory& pMemory);
  bool remove(node& key, shared_memory& pMemory);

  // map
  template <typename Key, typename Value>
  void force_insert(const Key& key, const Value& value, shared_memory& pMemory);

  static std::string empty_scalar;
  static std::string tag_none;
  static std::string tag_other;
  static std::string tag_non_plain_scalar;

 private:

  typedef std::vector<node*> node_seq;
  typedef std::vector<std::pair<node*, node*>> node_map;

  void free_data();

  std::string& scalar() {
    assert(m_type == NodeType::Scalar);
    return *reinterpret_cast<std::string*>(&m_data);
  }

  node_map& map() {
    assert(m_type == NodeType::Map);
    return *reinterpret_cast<node_map*>(&m_data);
  }
  const node_map& map() const{
    assert(m_type == NodeType::Map);
    return *reinterpret_cast<const node_map*>(&m_data);
  }

  node_seq& seq() {
    assert(m_type == NodeType::Sequence);
    return *reinterpret_cast<node_seq*>(&m_data);
  }
  const node_seq& seq() const {
    assert(m_type == NodeType::Sequence);
    return *reinterpret_cast<const node_seq*>(&m_data);
  }

  std::size_t compute_seq_size() const;
  std::size_t compute_map_size() const;

  void insert_map_pair(node& key, node& value);
  void convert_to_map(shared_memory& pMemory);
  void convert_sequence_to_map(shared_memory& pMemory);

  template <typename T>
  static node& convert_to_node(const T& rhs, shared_memory& pMemory);

 private:

  NodeType m_type;
  EmitterStyle m_style;
  mutable bool m_hasUndefined;

  Mark m_mark;

  using data = typename std::aligned_storage<
    static_max<sizeof(std::string),
               sizeof(node_seq),
               sizeof(node_map)>::value,
    static_max<alignof(std::string),
               alignof(node_seq),
               alignof(node_map)>::value>::type;

  data m_data;

  const std::string* m_tag;
};
}
}

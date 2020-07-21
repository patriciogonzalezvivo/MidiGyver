#include <assert.h>
#include <iterator>
#include <sstream>

#include "yaml-cpp/exceptions.h"
#include "yaml-cpp/node/detail/memory.h"
#include "yaml-cpp/node/detail/node.h"  // IWYU pragma: keep
#include "yaml-cpp/node/detail/node_data.h"
#include "yaml-cpp/node/detail/node_iterator.h"
#include "yaml-cpp/node/ptr.h"
#include "yaml-cpp/node/type.h"

namespace YAML {
namespace detail {

std::string node_data::empty_scalar = "";
std::string node_data::tag_none = "";
std::string node_data::tag_other = "!";
std::string node_data::tag_non_plain_scalar = "?";

node_data::node_data()
    : m_type(NodeType::Undefined),
      m_style(EmitterStyle::Default),
      m_hasUndefined(false),
      m_mark(Mark::null_mark()),
      m_tag(nullptr) {}

void node_data::mark_defined() {
  if (m_type == NodeType::Undefined)
    m_type = NodeType::Null;
}

node_data::~node_data() {
  if (m_tag &&
     m_tag != &tag_other &&
     m_tag != &tag_non_plain_scalar) {
    delete m_tag;
  }
  free_data();
}

void node_data::free_data() {
  switch (m_type) {
    case NodeType::Null:
      break;
    case NodeType::Scalar: {
      using namespace std;
      scalar().~string();
      break;
    }
    case NodeType::Sequence:
      seq().~node_seq();
      break;
    case NodeType::Map:
      map().~node_map();
      break;
    case NodeType::Undefined:
      break;
  }
}

void node_data::set_type(NodeType type) {

  if (type == m_type)
    return;

  if (m_type != NodeType::Undefined) {
    free_data();
  }

  m_type = type;

  switch (m_type) {
    case NodeType::Null:
      break;
    case NodeType::Scalar:
      new (&m_data) std::string;
      break;
    case NodeType::Sequence:
      new (&m_data) node_seq;
      seq().reserve(4);
      break;
    case NodeType::Map:
      new (&m_data) node_map;
      map().reserve(4);
      break;
    case NodeType::Undefined:
      break;
  }
}

void node_data::set_tag(const std::string& tag) {
  if (m_tag &&
      m_tag != &tag_other &&
      m_tag != &tag_non_plain_scalar) {
    delete m_tag;
  }

  if (tag == tag_other) {
    m_tag = &tag_other;
  } else if (tag == tag_non_plain_scalar) {
    m_tag = &tag_non_plain_scalar;
  } else {
    m_tag = new std::string(tag);
  }
}

void node_data::set_null() {
  set_type(NodeType::Null);
}

void node_data::set_scalar(const std::string& scalar_) {
  set_type(NodeType::Scalar);
  scalar() = scalar_;
}

void node_data::set_scalar(std::string&& scalar_) {
  if (m_type == NodeType::Scalar) {
    scalar() = std::move(scalar_);
    return;
  }
  if (m_type != NodeType::Undefined &&
      m_type != NodeType::Null) {
    free_data();
  }

  new (&m_data) std::string(std::move(scalar_));
  m_type = NodeType::Scalar;
}

// size/iterator
std::size_t node_data::size() const {
  if (!is_defined())
    return 0;

  switch (m_type) {
    case NodeType::Sequence:
      return compute_seq_size();;
    case NodeType::Map:
      return compute_map_size();
    default:
      return 0;
  }
  return 0;
}

std::size_t node_data::compute_seq_size() const {
  if (!m_hasUndefined) { return seq().size(); }
  std::size_t seqSize = 0;
  while (seqSize < seq().size() && seq()[seqSize]->is_defined())
    seqSize++;

  if (seqSize == seq().size()) { m_hasUndefined = false; }
  return seqSize;
}

std::size_t node_data::compute_map_size() const {
  if (!m_hasUndefined) { return map().size(); }

  std::size_t seqSize = 0;
  for (auto& it : map())  {
    if (it.first->is_defined() && it.second->is_defined()) {
      seqSize++;
    }
  }
  if (seqSize == map().size()) { m_hasUndefined = false; }
  return seqSize;
}

const_node_iterator node_data::begin() const {
  if (!is_defined())
    return const_node_iterator();

  switch (m_type) {
    case NodeType::Sequence:
      return const_node_iterator(seq().begin());
    case NodeType::Map:
      return const_node_iterator(map().begin(), map().end());
    default:
      return const_node_iterator();
  }
}

node_iterator node_data::begin() {
  switch (m_type) {
    case NodeType::Sequence:
      return node_iterator(seq().begin());
    case NodeType::Map:
      return node_iterator(map().begin(), map().end());
    default:
      return node_iterator();
  }
}

const_node_iterator node_data::end() const {
  switch (m_type) {
    case NodeType::Sequence:
      return const_node_iterator(seq().end());
    case NodeType::Map:
      return const_node_iterator(map().end(), map().end());
    default:
      return const_node_iterator();
  }
}

node_iterator node_data::end() {
  switch (m_type) {
    case NodeType::Sequence:
      return node_iterator(seq().end());
    case NodeType::Map:
      return node_iterator(map().end(), map().end());
    default:
      return node_iterator();
  }
}

// sequence
void node_data::push_back(node& node, shared_memory& /* pMemory */) {

  if (m_type == NodeType::Undefined || m_type == NodeType::Null) {
    set_type(NodeType::Sequence);
  }

  if (m_type != NodeType::Sequence)
    throw BadPushback();

  seq().push_back(&node);

  if (!node.is_defined()) {
    m_hasUndefined = true;
  }
}

void node_data::insert(node& key, node& value, shared_memory& pMemory) {
  switch (m_type) {
    case NodeType::Map:
      break;
    case NodeType::Undefined:
    case NodeType::Null:
    case NodeType::Sequence:
      convert_to_map(pMemory);
      break;
    case NodeType::Scalar:
      throw BadSubscript();
  }

  insert_map_pair(key, value);
}

// indexing
node* node_data::get(node& key, shared_memory& /* pMemory */) const {
  if (m_type != NodeType::Map) {
    return nullptr;
  }

  for (node_map::const_iterator it = map().begin(); it != map().end(); ++it) {
    if (it->first->is(key))
      return it->second;
  }

  return nullptr;
}

node& node_data::get(node& key, shared_memory& pMemory) {
  switch (m_type) {
    case NodeType::Map:
      break;
    case NodeType::Undefined:
    case NodeType::Null:
    case NodeType::Sequence:
      convert_to_map(pMemory);
      break;
    case NodeType::Scalar:
      throw BadSubscript();
  }
  for (node_map::const_iterator it = map().begin(); it != map().end(); ++it) {
    if (it->first->is(key))
      return *it->second;
  }

  node& value = pMemory->create_node();
  insert_map_pair(key, value);
  return value;
}

bool node_data::remove(node& key, shared_memory& /* pMemory */) {
  if (m_type != NodeType::Map)
    return false;

  for (node_map::iterator it = map().begin(); it != map().end(); ++it) {
    if (it->first->is(key)) {
      map().erase(it);
      return true;
    }
  }
  return false;
}

void node_data::insert_map_pair(node& key, node& value) {
  map().emplace_back(&key, &value);

  if (!key.is_defined() || !value.is_defined()) {
    m_hasUndefined = true;
  }
}

void node_data::convert_to_map(shared_memory& pMemory) {
  switch (m_type) {
    case NodeType::Undefined:
    case NodeType::Null:
      set_type(NodeType::Map);
      break;
    case NodeType::Sequence:
      convert_sequence_to_map(pMemory);
      break;
    case NodeType::Map:
      break;
    case NodeType::Scalar:
      assert(false);
      break;
  }
}

void node_data::convert_sequence_to_map(shared_memory& pMemory) {

  node_seq tmp = std::move(seq());

  set_type(NodeType::Map);

  for (std::size_t i = 0; i < tmp.size(); i++) {
    std::stringstream stream;
    stream << i;

    node& key = pMemory->create_node();
    key.set_scalar(stream.str());
    insert_map_pair(key, *tmp[i]);
  }
}

}
}

#pragma once

#include <list>
#include <memory>

#include "yaml-cpp/dll.h"
#include "yaml-cpp/node/ptr.h"

namespace YAML {
namespace detail {
class node;
struct node_bucket;
}  // namespace detail
}  // namespace YAML

namespace YAML {
namespace detail {

class YAML_CPP_API memory : public ref_counted {
 public:
  node& create_node();
  void merge(memory& rhs);

  memory();
  ~memory();

 private:
  std::unique_ptr<node_bucket> buckets;
};

struct memory_ref : ref_counted {

  ref_holder<memory, true> m_ptr;

  memory_ref() : m_ptr(new memory) {}
  ~memory_ref() {}

  node& create_node() { return m_ptr->create_node(); }

  void merge(memory_ref& rhs) {
    if (m_ptr == rhs.m_ptr) {
       return;
    }
    m_ptr->merge(*rhs.m_ptr);
    rhs.m_ptr = m_ptr;
  };
};

typedef ref_holder<memory_ref, true> shared_memory;

}
}

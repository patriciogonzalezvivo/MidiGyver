#include "yaml-cpp/node/detail/memory.h"
#include "yaml-cpp/node/detail/node.h"
#include "yaml-cpp/node/ptr.h"

#include <vector>

namespace YAML {
namespace detail {

struct node_bucket {
    static const size_t size = 64;
    node_bucket(node_bucket* next_, size_t capacity) : next(next_) {
        nodes.reserve(capacity);
    }

    ~node_bucket();
    void clear();

    struct value {
        node n;
        std::aligned_storage<sizeof(node_data),
                             alignof(node_data)>::type data;

        value() {
            new (&data) node_data;

            n.set_data(reinterpret_cast<node_data*>(&data));
        }
    };
    std::vector<value> nodes;
    std::unique_ptr<node_bucket> next = nullptr;
};

node_bucket::~node_bucket() {}

node& memory::create_node() {
    node_bucket* insert = buckets.get();

    for (node_bucket* b = insert; b; b = b->next.get()) {
        if (b->nodes.size() == b->nodes.capacity()) {
            b = nullptr;
            break;
        }
        insert = b;
    }

    if (insert && insert->nodes.size() < insert->nodes.capacity()) {
        insert->nodes.emplace_back();
        return insert->nodes.back().n;
    }

    if (!buckets) {
        buckets = std::unique_ptr<node_bucket>(new node_bucket(nullptr, 8));
    } else {
        buckets = std::unique_ptr<node_bucket>(new node_bucket(buckets.release(), node_bucket::size));
    }
    buckets->nodes.emplace_back();
    return buckets->nodes.back().n;
}

void memory::merge(memory& rhs) {

    if (rhs.buckets.get() == buckets.get()) {
        return;
    }
    if (!rhs.buckets) {
        return;
    }

    if (!buckets) {
        buckets.reset(rhs.buckets.release());
        return;
    }

    // last before filled bucket
    node_bucket* insert = nullptr;
    for (node_bucket* b = buckets.get(); b; b = b->next.get()) {
        if (b->nodes.size() == b->nodes.capacity()) {
            break;
        }
        insert = b;
    }

    node_bucket* last = rhs.buckets.get();
    for (node_bucket* b = last; b; b = b->next.get()) {
        last = b;
    }

    node_bucket* appendix = nullptr;
    if (insert) {
        appendix = insert->next.release();
        insert->next.reset(rhs.buckets.release());
    } else {
        appendix = buckets.release();
        buckets.reset(rhs.buckets.release());

    }

    if (appendix) {
        last->next.reset(appendix);
    }
}

memory::memory() {}
memory::~memory() {
  // Important:
  // First clear all node_data refs
  for (node_bucket* b = buckets.get(); b; b = b->next.get()) {
    b->nodes.clear();
  }
  // Then delete buckets
  while (buckets) {
    buckets = std::move(buckets->next);
  }
}
}
}

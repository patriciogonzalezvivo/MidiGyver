#pragma once

#include <stack>
#include <vector>
#include <cassert>

namespace YAML {
struct CollectionType {
  enum value { NoCollection, BlockMap, BlockSeq, FlowMap, FlowSeq, CompactMap };
};

class CollectionStack {
 public:
  CollectionType::value GetCurCollectionType() const {
    if (collectionStack.empty())
      return CollectionType::NoCollection;
    return collectionStack.top();
  }

  void PushCollectionType(CollectionType::value type) {
    collectionStack.push(type);
  }
  void PopCollectionType(CollectionType::value type) {
    assert(type == GetCurCollectionType());
    (void)type;
    collectionStack.pop();
  }

 private:
  std::stack<CollectionType::value, std::vector<CollectionType::value>> collectionStack;
};
}

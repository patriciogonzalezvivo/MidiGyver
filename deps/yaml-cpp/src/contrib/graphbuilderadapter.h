#pragma once

#include <cstdlib>
#include <map>
#include <stack>

#include "yaml-cpp/anchor.h"
#include "yaml-cpp/contrib/anchordict.h"
#include "yaml-cpp/contrib/graphbuilder.h"
#include "yaml-cpp/emitterstyle.h"
#include "yaml-cpp/eventhandler.h"

namespace YAML {
class GraphBuilderInterface;
struct Mark;
}  // namespace YAML

namespace YAML {
class GraphBuilderAdapter : public EventHandler {
 public:
  GraphBuilderAdapter(GraphBuilderInterface& builder)
      : m_builder(builder), m_pRootNode(nullptr), m_pKeyNode(nullptr) {}

  void OnDocumentStart(const Mark& mark) override { (void)mark; }
  void OnDocumentEnd() override {}

  void OnNull(const Mark& mark, anchor_t anchor) override;
  void OnAlias(const Mark& mark, anchor_t anchor) override;
  void OnScalar(const Mark& mark, const std::string& tag,
                anchor_t anchor, std::string value) override;

  void OnSequenceStart(const Mark& mark, const std::string& tag,
                       anchor_t anchor, EmitterStyle style) override;
  void OnSequenceEnd() override;

  void OnMapStart(const Mark& mark, const std::string& tag,
                  anchor_t anchor, EmitterStyle style) override;
  void OnMapEnd() override;

  void* RootNode() const { return m_pRootNode; }

 private:
  struct ContainerFrame {
    ContainerFrame(void* pSequence)
        : pContainer(pSequence), pPrevKeyNode(&sequenceMarker) {}
    ContainerFrame(void* pMap, void* pPrevKeyNode)
        : pContainer(pMap), pPrevKeyNode(pPrevKeyNode) {}

    void* pContainer;
    void* pPrevKeyNode;

    bool isMap() const { return pPrevKeyNode != &sequenceMarker; }

   private:
    static int sequenceMarker;
  };
  typedef std::stack<ContainerFrame> ContainerStack;
  typedef AnchorDict<void*> AnchorMap;

  GraphBuilderInterface& m_builder;
  ContainerStack m_containers;
  AnchorMap m_anchors;
  void* m_pRootNode;
  void* m_pKeyNode;

  void* GetCurrentParent() const;
  void RegisterAnchor(anchor_t anchor, void* pNode);
  void DispositionNode(void* pNode);
};
}

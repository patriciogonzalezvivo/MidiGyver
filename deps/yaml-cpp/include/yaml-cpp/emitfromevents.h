#pragma once

#include <stack>

#include "yaml-cpp/anchor.h"
#include "yaml-cpp/emitterstyle.h"
#include "yaml-cpp/eventhandler.h"

namespace YAML {
struct Mark;
}  // namespace YAML

namespace YAML {
class Emitter;

class EmitFromEvents : public EventHandler {
 public:
  EmitFromEvents(Emitter& emitter);

  void OnDocumentStart(const Mark& mark) override;
  void OnDocumentEnd() override;

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

 private:
  void BeginNode();
  void EmitProps(const std::string& tag, anchor_t anchor);

 private:
  Emitter& m_emitter;

  enum class State : char { WaitingForSequenceEntry, WaitingForKey, WaitingForValue };

  std::stack<State> m_stateStack;
};
}

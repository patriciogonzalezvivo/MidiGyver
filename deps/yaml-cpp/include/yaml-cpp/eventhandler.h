#pragma once

#include <string>

#include "yaml-cpp/anchor.h"
#include "yaml-cpp/emitterstyle.h"

namespace YAML {
struct Mark;

class EventHandler {
 public:
  virtual ~EventHandler() {}

  virtual void OnDocumentStart(const Mark& mark) = 0;
  virtual void OnDocumentEnd() = 0;

  virtual void OnNull(const Mark& mark, anchor_t anchor) = 0;
  virtual void OnAlias(const Mark& mark, anchor_t anchor) = 0;
  virtual void OnScalar(const Mark& mark, const std::string& tag,
                        anchor_t anchor, std::string value) = 0;

  virtual void OnSequenceStart(const Mark& mark, const std::string& tag,
                               anchor_t anchor, EmitterStyle style) = 0;
  virtual void OnSequenceEnd() = 0;

  virtual void OnMapStart(const Mark& mark, const std::string& tag,
                          anchor_t anchor, EmitterStyle style) = 0;
  virtual void OnMapEnd() = 0;
};

}

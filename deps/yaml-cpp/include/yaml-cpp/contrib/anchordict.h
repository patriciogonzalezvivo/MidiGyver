#pragma once

#include <vector>

#include "../anchor.h"

namespace YAML {
/**
 * An object that stores and retrieves values correlating to {@link anchor_t}
 * values.
 *
 * <p>Efficient implementation that can make assumptions about how
 * {@code anchor_t} values are assigned by the {@link Parser} class.
 */
template <class T>
class AnchorDict {
 public:
  void Register(anchor_t anchor, T value) {
    if (anchor > m_data.size()) {
      m_data.resize(anchor);
    }
    m_data[anchor - 1] = value;
  }

  T Get(anchor_t anchor) const { return m_data[anchor - 1]; }

 private:
  std::vector<T> m_data;
};
}

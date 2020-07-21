#pragma once

#include <string>
#include <iosfwd>

#include "yaml-cpp/dll.h"

namespace YAML {
class Emitter;
class Node;

/**
 * Emits the node to the given {@link Emitter}. If there is an error in writing,
 * {@link Emitter#good} will return false.
 */
YAML_CPP_API Emitter& operator<<(Emitter& out, const Node& node);

/** Emits the node to the given output stream. */
YAML_CPP_API std::ostream& operator<<(std::ostream& out, const Node& node);

/** Converts the node to a YAML string. */
YAML_CPP_API std::string Dump(const Node& node);
} // namespace YAML

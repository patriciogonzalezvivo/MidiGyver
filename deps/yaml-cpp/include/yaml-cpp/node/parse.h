#pragma once

#include <iosfwd>
#include <string>
#include <vector>

#include "yaml-cpp/dll.h"

namespace YAML {
class Node;

/**
 * Loads the input string as a single YAML document.
 *
 * @throws {@link ParserException} if it is malformed.
 */
YAML_CPP_API Node Load(const std::string& input);

/**
 * Loads the input string as a single YAML document.
 *
 * @throws {@link ParserException} if it is malformed.
 */
YAML_CPP_API Node Load(const char* input, size_t length);

/**
 * Loads the input stream as a single YAML document.
 *
 * @throws {@link ParserException} if it is malformed.
 */
YAML_CPP_API Node Load(std::istream& input);

/**
 * Loads the input file as a single YAML document.
 *
 * @throws {@link ParserException} if it is malformed.
 * @throws {@link BadFile} if the file cannot be loaded.
 */
YAML_CPP_API Node LoadFile(const std::string& filename);

/**
 * Loads the input string as a list of YAML documents.
 *
 * @throws {@link ParserException} if it is malformed.
 */
YAML_CPP_API std::vector<Node> LoadAll(const std::string& input);

/**
 * Loads the input stream as a list of YAML documents.
 *
 * @throws {@link ParserException} if it is malformed.
 */
YAML_CPP_API std::vector<Node> LoadAll(std::istream& input);

/**
 * Loads the input file as a list of YAML documents.
 *
 * @throws {@link ParserException} if it is malformed.
 * @throws {@link BadFile} if the file cannot be loaded.
 */
YAML_CPP_API std::vector<Node> LoadAllFromFile(const std::string& filename);
}  // namespace YAML

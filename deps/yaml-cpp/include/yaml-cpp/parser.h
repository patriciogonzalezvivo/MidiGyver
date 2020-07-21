#pragma once

#include <ios>
#include <memory>

#include "yaml-cpp/dll.h"
#include "yaml-cpp/noncopyable.h"

namespace YAML {
class EventHandler;
class Node;
class Scanner;
struct Directives;
struct Token;

/**
 * A parser turns a stream of bytes into one stream of "events" per YAML
 * document in the input stream.
 */
class YAML_CPP_API Parser : private noncopyable {
 public:
  /** Constructs an empty parser (with no input. */
  Parser();

  /**
   * Constructs a parser from the given input stream. The input stream must
   * live as long as the parser.
   */
  explicit Parser(std::istream& in);

  ~Parser();

  /** Evaluates to true if the parser has some valid input to be read. */
  explicit operator bool() const;

  /**
   * Resets the parser with the given input stream. Any existing state is
   * erased.
   */
  void Load(std::istream& in);

  /**
   * Handles the next document by calling events on the {@code eventHandler}.
   *
   * @throw a ParserException on error.
   * @return false if there are no more documents
   */
  bool HandleNextDocument(EventHandler& eventHandler);

  void PrintTokens(std::ostream& out);

 private:
  /**
   * Reads any directives that are next in the queue, setting the internal
   * {@code m_pDirectives} state.
   */
  void ParseDirectives();

  void HandleDirective(const Token& token);

  /**
   * Handles a "YAML" directive, which should be of the form 'major.minor' (like
   * a version number).
   */
  void HandleYamlDirective(const Token& token);

  /**
   * Handles a "TAG" directive, which should be of the form 'handle prefix',
   * where 'handle' is converted to 'prefix' in the file.
   */
  void HandleTagDirective(const Token& token);

 private:
  std::unique_ptr<Scanner> m_pScanner;
  std::unique_ptr<Directives> m_pDirectives;
};
}

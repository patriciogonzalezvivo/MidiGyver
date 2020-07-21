#pragma once

#include <cstddef>
#include <ios>
#include <map>
#include <memory>
#include <deque>
#include <set>
#include <stack>
#include <string>
#include <list>
#include <forward_list>

#include "scanscalar.h"
#include "stream.h"
#include "token.h"
#include "yaml-cpp/mark.h"
#include "exp.h"
#include "plalloc.h"

namespace YAML {
class Node;
class RegEx;

/**
 * A scanner transforms a stream of characters into a stream of tokens.
 */
class Scanner {
 public:
  explicit Scanner(std::istream &in);
  ~Scanner();

  /** Returns true if there are no more tokens to be read. */
  bool empty();

  /** Removes the next token in the queue. */
  void pop();

  void pop_unsafe() {
    ++m_tokenOut;
  }

  Token& push() {
    if (m_tokenOut != m_tokens.begin()) {
      // Move free token to the end
      auto last = m_tokens.begin();
      m_tokens.splice_after(m_tokenIn, m_tokens, m_tokens.before_begin());
      m_tokenIn = last;
    } else {
      // Full
      CreateToken();
    }
    if (m_tokenOut == m_tokens.end()) {
      m_tokenOut = m_tokenIn;
    }

    m_tokenIn->status = Token::VALID;
    return *m_tokenIn;
  }

  /** Returns, but does not remove, the next token in the queue. */
  Token &peek();

  Token &peek_unsafe() {
    return *m_tokenPtr;
  }

  /** Returns the current mark in the input stream. */
  Mark mark() const;

 private:
  struct IndentMarker {
    enum INDENT_TYPE { MAP, SEQ, NONE };
    enum STATUS { VALID, INVALID, UNKNOWN };
    IndentMarker(int column_, INDENT_TYPE type_)
        : column(column_), type(type_), status(VALID), pStartToken(nullptr) {}

    int column;
    INDENT_TYPE type;
    STATUS status;
    Token *pStartToken;
  };

  enum FLOW_MARKER { FLOW_MAP, FLOW_SEQ };

 private:
  // scanning

  /**
   * Scans until there's a valid token at the front of the queue, or the queue
   * is empty. The state can be checked by {@link #empty}, and the next token
   * retrieved by {@link #peek}.
   */
  void EnsureTokensInQueue();

  /**
   * The main scanning function; this method branches out to scan whatever the
   * next token should be.
   */
  void ScanNextToken();

  /** Eats the input stream until it reaches the next token-like thing. */
  void ScanToNextToken();

  /** Sets the initial conditions for starting a stream. */
  void StartStream();

  /** Closes out the stream, finish up, etc. */
  void EndStream();


  inline bool InFlowContext() const { return !m_flows.empty(); }
  bool InBlockContext() const { return m_flows.empty(); }
  std::size_t GetFlowLevel() const { return m_flows.size(); }

  Token::TYPE GetStartTokenFor(IndentMarker::INDENT_TYPE type) const;

  /**
   * Pushes an indentation onto the stack, and enqueues the proper token
   * (sequence start or mapping start).
   *
   * @return the indent marker it generates (if any).
   */
  IndentMarker *PushIndentTo(int column, IndentMarker::INDENT_TYPE type);

  /**
   * Pops indentations off the stack until it reaches the current indentation
   * level, and enqueues the proper token each time. Then pops all invalid
   * indentations off.
   */
  void PopIndentToHere();

  /**
   * Pops all indentations (except for the base empty one) off the stack, and
   * enqueues the proper token each time.
   */
  void PopAllIndents();

  /** Pops a single indent, pushing the proper token. */
  void PopIndent();

  inline int GetTopIndent() const {
    if (m_indents.empty()) { return 0; }
    return m_indents.top()->column;
  }
  // checking input
  bool CanInsertPotentialSimpleKey() const;
  bool ExistsActiveSimpleKey() const;
  void InsertPotentialSimpleKey();
  void InvalidateSimpleKey();
  bool VerifySimpleKey();
  void PopAllSimpleKeys();

  /**
   * Throws a ParserException with the current token location (if available),
   * and does not parse any more tokens.
   */
  void ThrowParserException(const std::string &msg) const;

  bool IsWhitespaceToBeEaten(char ch);


  struct SimpleKey {
    SimpleKey(const Mark &mark_, std::size_t flowLevel_);

    void Validate();
    void Invalidate();

    int markPos;
    int markLine;
    std::size_t flowLevel;
    IndentMarker *pIndent;
    Token *pMapStart, *pKey;
  };

  // and the tokens
  void ScanDirective();
  void ScanDocStart();
  void ScanDocEnd();
  void ScanBlockSeqStart();
  void ScanBlockMapSTart();
  void ScanBlockEnd();
  void ScanBlockEntry();
  void ScanFlowStart();
  void ScanFlowEnd();
  void ScanFlowEntry();
  void ScanKey();
  void ScanValue();
  void ScanAnchorOrAlias();
  void ScanTag();
  void ScanPlainScalar();
  void ScanQuotedScalar();
  void ScanBlockScalar();

  // scanscalar.cpp
  std::string ScanScalar(ScanScalarParams& info);
  static int MatchScalarEmpty(Exp::Source<4> in);
  static int MatchScalarSingleQuoted(Exp::Source<4> in);
  static int MatchScalarDoubleQuoted(Exp::Source<4> in);
  static int MatchScalarEnd(Exp::Source<4> in);
  static int MatchScalarEndInFlow(Exp::Source<4> in);
  static int MatchScalarIndent(Exp::Source<4> in);

 private:

  void CreateToken();
  void InitTokens();

  // the stream
  Stream INPUT;

  // the output (tokens)
  template<typename T>
  using token_alloc = plalloc<T, 64>;

  std::forward_list<Token, token_alloc<Token>> m_tokens;
  //std::forward_list<Token> m_tokens;
  std::forward_list<Token>::iterator m_tokenOut;
  std::forward_list<Token>::iterator m_tokenIn;
  Token* m_tokenPtr = nullptr;

  // state info
  bool m_startedStream, m_endedStream;
  bool m_simpleKeyAllowed;
  bool m_canBeJSONFlow;
  std::stack<SimpleKey,std::vector<SimpleKey>> m_simpleKeys;
  std::stack<IndentMarker *,std::vector<IndentMarker *>> m_indents;
  std::deque<IndentMarker> m_indentRefs;  // for "garbage collection"
  std::stack<FLOW_MARKER, std::vector<FLOW_MARKER>> m_flows;

  std::string m_scalarBuffer;
};
}

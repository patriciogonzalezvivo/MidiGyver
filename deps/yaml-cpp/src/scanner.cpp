#include <cassert>
#include <memory>

#include "exp.h"
#include "scanner.h"
#include "token.h"
#include "yaml-cpp/exceptions.h"  // IWYU pragma: keep

namespace YAML {
Scanner::Scanner(std::istream& in)
    : INPUT(in),
      m_startedStream(false),
      m_endedStream(false),
      m_simpleKeyAllowed(false),
      m_canBeJSONFlow(false) {
  InitTokens();
}

Scanner::~Scanner() {}

bool Scanner::empty() {
  EnsureTokensInQueue();
  return m_tokenOut == m_tokens.end();
}

void Scanner::pop() {
  if (!empty()) {
    pop_unsafe();
  }
}

Token& Scanner::peek() {
  EnsureTokensInQueue();

  // should we be asserting here? I mean, we really
  // just be checking if it's empty before peeking.
  assert(m_tokenOut != m_tokens.end());

#if 0
  static Token *pLast = 0;
  if(pLast != &m_tokens.front())
    std::cerr << "peek: " << m_tokens.front() << "\n";
  pLast = &m_tokens.front();
#endif

  return peek_unsafe();
}

void Scanner::CreateToken() {
  m_tokenIn = m_tokens.emplace_after(m_tokenIn);
}

void Scanner::InitTokens() {
  m_tokenOut = m_tokens.end();
  for (int i = 0; i < 64; i++) {
    m_tokens.emplace_front();
    if (i == 0) { m_tokenIn = m_tokens.begin(); }
  }

  // auto prev = m_tokens.begin();
  // auto it = m_tokens.begin();
  // ++it;
  // for (; it != m_tokens.end(); it++) {
  //   printf("%li ", (std::ptrdiff_t(&(*it)) - std::ptrdiff_t(&(*prev))));
  //   prev = it;
  // }
  // printf("\n -- %lu / %lu\n", sizeof(Token), alignof(Token));
}

Mark Scanner::mark() const { return INPUT.mark(); }

void Scanner::EnsureTokensInQueue() {
  while (1) {
    if (m_tokenOut != m_tokens.end()) {
      Token& token = *m_tokenOut;
      m_tokenPtr = &token;

      // if this guy's valid, then we're done
      if (token.status == Token::VALID) {
        return;
      }

      // here's where we clean up the impossible tokens
      if (token.status == Token::INVALID) {
        pop_unsafe();
        continue;
      }

      // note: what's left are the unverified tokens
    }

    // no token? maybe we've actually finished
    if (m_endedStream) {
      return;
    }

    // no? then scan...
    ScanNextToken();
  }

  m_tokenPtr = nullptr;
}

void Scanner::ScanNextToken() {
  if (m_endedStream) {
    return;
  }

  if (!m_startedStream) {
    return StartStream();
  }

  // get rid of whitespace, etc. (in between tokens it should be irrelevent)
  ScanToNextToken();

  // maybe need to end some blocks
  PopIndentToHere();

  // *****
  // And now branch based on the next few characters!
  // *****

  // end of stream
  if (!INPUT) {
    return EndStream();
  }
  char c = INPUT.peek();

  // flow start/end/entry
  if (c == Keys::FlowSeqStart ||
      c == Keys::FlowMapStart) {
    return ScanFlowStart();

  } else if (c == Keys::FlowSeqEnd ||
      c == Keys::FlowMapEnd) {
    return ScanFlowEnd();

  } else if (c == Keys::FlowEntry) {
    return ScanFlowEntry();
  }

  if (INPUT.column() == 0) {
    if (c == Keys::Directive) {
      return ScanDirective();
    }

    // document token
    if (Exp::DocStart::Matches(INPUT)) {
        return ScanDocStart();
    }

    if (Exp::DocEnd::Matches(INPUT)) {
        return ScanDocEnd();
    }
  }

  // Get large enough lookahead buffer for all Matchers
  Exp::Source<4> input;
  INPUT.LookaheadBuffer(input);
  // block/map stuff
  if (Exp::BlockEntry::Matches(input)) {
    ScanBlockEntry();

  } else if (InBlockContext() ?
      // TODO these are the same?
      Exp::Key::Matches(input) :
      Exp::KeyInFlow::Matches(input)) {
    ScanKey();

  } else if (InBlockContext() && Exp::Value::Matches(input)) {
    ScanValue();

  } else if (m_canBeJSONFlow && Exp::ValueInJSONFlow::Matches(input)) {
    ScanValue();

  } else if (!m_canBeJSONFlow && Exp::ValueInFlow::Matches(input)) {
    ScanValue();

  }  else if (c == Keys::Alias || c == Keys::Anchor) {
    ScanAnchorOrAlias();

  } else if (c == Keys::Tag) {
    ScanTag();

  } else if (InBlockContext() && (c == Keys::LiteralScalar ||
                                  c == Keys::FoldedScalar)) {
    // special scalars
    ScanBlockScalar();

  } else if  (c == '\'' || c == '\"') {
    ScanQuotedScalar();

  } else if (Exp::PlainScalarCommon::Matches(input) &&
             ((InBlockContext() && Exp::PlainScalar::Matches(input)) ||
              (InFlowContext() && Exp::PlainScalarInFlow::Matches(input)))) {

    ScanPlainScalar();

  } else {
    // don't know what it is!
    throw ParserException(INPUT.mark(), ErrorMsg::UNKNOWN_TOKEN);
  }
}

void Scanner::ScanToNextToken() {
  while (1) {
    INPUT.EatSpace();

    // first eat whitespace
    while (INPUT && IsWhitespaceToBeEaten(INPUT.peek())) {
      if (InBlockContext() && Exp::Tab::Matches(INPUT)) {
        m_simpleKeyAllowed = false;
      }
      INPUT.eat();
    }

    // then eat a comment
    if (Exp::Comment::Matches(INPUT)) {
      // eat until line break
      INPUT.EatToEndOfLine();
    }

    // if it's NOT a line break, then we're done!
    // otherwise, let's eat the line break and keep going
    if (!(INPUT.peek() == '\n' || INPUT.peek() == '\r') ||
        !INPUT.EatLineBreak()) {
        break;
    }

    // oh yeah, and let's get rid of that simple key
    InvalidateSimpleKey();

    // new line - we may be able to accept a simple key now
    if (InBlockContext()) {
      m_simpleKeyAllowed = true;
    }
  }
}

///////////////////////////////////////////////////////////////////////
// Misc. helpers

// IsWhitespaceToBeEaten
// . We can eat whitespace if it's a space or tab
// . Note: originally tabs in block context couldn't be eaten
//         "where a simple key could be allowed
//         (i.e., not at the beginning of a line, or following '-', '?', or
// ':')"
//   I think this is wrong, since tabs can be non-content whitespace; it's just
//   that they can't contribute to indentation, so once you've seen a tab in a
//   line, you can't start a simple key
bool Scanner::IsWhitespaceToBeEaten(char ch) {
  if (ch == ' ') {
    return true;
  }

  if (ch == '\t') {
    return true;
  }

  return false;
}


void Scanner::StartStream() {
  m_startedStream = true;
  m_simpleKeyAllowed = true;

  m_indentRefs.emplace_back(-1, IndentMarker::NONE);
  m_indents.push(&m_indentRefs.back());
}

void Scanner::EndStream() {
  // force newline
  if (INPUT.column() > 0) {
    INPUT.ResetColumn();
  }

  PopAllIndents();
  PopAllSimpleKeys();

  m_simpleKeyAllowed = false;
  m_endedStream = true;
}

Token::TYPE Scanner::GetStartTokenFor(IndentMarker::INDENT_TYPE type) const {
  switch (type) {
    case IndentMarker::SEQ:
      return Token::BLOCK_SEQ_START;
    case IndentMarker::MAP:
      return Token::BLOCK_MAP_START;
    case IndentMarker::NONE:
      assert(false);
      break;
  }
  assert(false);
  throw std::runtime_error("yaml-cpp: internal error, invalid indent type");
}

Scanner::IndentMarker* Scanner::PushIndentTo(int column,
                                             IndentMarker::INDENT_TYPE type) {
  // are we in flow?
  if (InFlowContext()) {
    return nullptr;
  }

  const IndentMarker& lastIndent = *m_indents.top();

  // is this actually an indentation?
  if (column < lastIndent.column) {
    return nullptr;
  }
  if (column == lastIndent.column &&
      !(type == IndentMarker::SEQ &&
        lastIndent.type == IndentMarker::MAP)) {
    return nullptr;
  }

  m_indentRefs.emplace_back(column, type);
  IndentMarker& indent = m_indentRefs.back();

  // push a start token
  auto& token = push();
  token.type = GetStartTokenFor(type);
  token.mark = INPUT.mark();
  indent.pStartToken = &token;

  // and then the indent
  m_indents.push(&indent);
  return &m_indentRefs.back();
}

void Scanner::PopIndentToHere() {
  // are we in flow?
  if (InFlowContext()) {
    return;
  }

  // now pop away
  while (!m_indents.empty()) {
    const IndentMarker& indent = *m_indents.top();
    if (indent.column < INPUT.column()) {
      break;
    }
    if (indent.column == INPUT.column() &&
        !(indent.type == IndentMarker::SEQ &&
          !Exp::BlockEntry::Matches(INPUT))) {
      break;
    }

    PopIndent();
  }

  while (!m_indents.empty() &&
         m_indents.top()->status == IndentMarker::INVALID) {
    PopIndent();
  }
}

void Scanner::PopAllIndents() {
  // are we in flow?
  if (InFlowContext()) {
    return;
  }

  // now pop away
  while (!m_indents.empty()) {
    const IndentMarker& indent = *m_indents.top();
    if (indent.type == IndentMarker::NONE) {
      break;
    }

    PopIndent();
  }
}

void Scanner::PopIndent() {
  const IndentMarker& indent = *m_indents.top();
  m_indents.pop();

  if (indent.status != IndentMarker::VALID) {
    InvalidateSimpleKey();
    return;
  }

  auto& token = push();
  token.mark = INPUT.mark();

  if (indent.type == IndentMarker::SEQ) {
    token.type = Token::BLOCK_SEQ_END;
  } else if (indent.type == IndentMarker::MAP) {
    token.type = Token::BLOCK_MAP_END;
  }
}

void Scanner::ThrowParserException(const std::string& msg) const {
  Mark mark = Mark::null_mark();
  if (m_tokenOut != m_tokens.end()) {
    const Token& token = *m_tokenOut;
    mark = token.mark;
  }
  throw ParserException(mark, msg);
}
}  // namespace YAML

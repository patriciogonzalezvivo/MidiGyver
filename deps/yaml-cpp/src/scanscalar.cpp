#include "scanner.h"

#include <algorithm>

#include "exp.h"
#include "stream.h"
#include "yaml-cpp/exceptions.h"  // IWYU pragma: keep

namespace YAML {

int Scanner::MatchScalarEmpty(Exp::Source<4>) {
  // This is checked by !INPUT as well
  return -1;
}

int Scanner::MatchScalarSingleQuoted(Exp::Source<4> in) {
  using namespace Exp;
  return (Matcher<Char<'\''>>::Matches(in) &&
          !EscSingleQuote::Matches(in)) ? 1 : -1;
}

int Scanner::MatchScalarDoubleQuoted(Exp::Source<4> in) {
  using namespace Exp;
  return Matcher<Char<'\"'>>::Match(in);
}

int Scanner::MatchScalarEnd(Exp::Source<4> in) {
  using namespace Exp;
  using ScalarEnd = Matcher<
      OR < SEQ < Char<':'>,
                 OR < detail::BlankOrBreak, Empty>>,
           SEQ < detail::BlankOrBreak,
                 detail::Comment>>>;

  return ScalarEnd::Match(in);
}

int Scanner::MatchScalarEndInFlow(Exp::Source<4> in) {
  using namespace Exp;
  using ScalarEndInFlow = Matcher <
      OR < SEQ < Char<':'>,
                 OR < detail::Blank,
                      Char<','>,
                      Char<']'>,
                      Char<'}'>,
                      detail::Break,
                      Empty >>,
           Char<','>,
           Char<'?'>,
           Char<'['>,
           Char<']'>,
           Char<'{'>,
           Char<'}'>,
           SEQ < detail::BlankOrBreak,
                 detail::Comment>>>;

   return ScalarEndInFlow::Match(in);
}

int Scanner::MatchScalarIndent(Exp::Source<4> in) {
  using namespace Exp;
  using ScalarEndInFlow = Matcher <
      SEQ < detail::Blank,
            detail::Comment>>;

  return ScalarEndInFlow::Match(in);
}

static bool MatchDocIndicator(const Stream& in) {
 using namespace Exp;
 using DocIndicator = Matcher<OR <detail::DocStart, detail::DocEnd>>;

 return DocIndicator::Matches(in);
}


struct ScanResult {
  bool foundNonEmptyLine = false;
  bool escapedNewline = false;
  int endMatch = -1;
  std::size_t lastNonWhitespaceChar;
  std::size_t lastEscapedChar = std::string::npos;
};

static void EatToIndentation(Stream& INPUT, ScanScalarParams& params, bool foundEmptyLine);

static void EatAfterIndentation(Stream& INPUT, ScanScalarParams& params);

static void PostProcess(std::string& scalar, ScanScalarParams& params, size_t lastEscapedChar);

static int HandleFolding(std::string& scalar, const ScanScalarParams& params,
                         int column, bool escapedNewline,
                         bool emptyLine, bool moreIndented,
                         bool nextEmptyLine, bool nextMoreIndented,
                         bool foundNonEmptyLine,
                         bool foldedNewlineStartedMoreIndented,
                         int foldedNewlineCount);

static void ScanLine(Stream& INPUT, const ScanScalarParams& params,
                     std::string& scalar, ScanResult& out);
//#define TEST_NO_INLINE __attribute__((noinline))
#define TEST_NO_INLINE

// ScanScalar
// . This is where the scalar magic happens.
//
// . We do the scanning in three phases:
//   1. Scan until newline
//   2. Eat newline
//   3. Scan leading blanks.
//
// . Depending on the parameters given, we store or stop
//   and different places in the above flow.
std::string Scanner::ScanScalar(ScanScalarParams& params) {

  bool emptyLine = false;
  bool moreIndented = false;
  bool foldedNewlineStartedMoreIndented = false;
  bool pastOpeningBreak = (params.fold == FOLD_FLOW);

  int foldedNewlineCount = 0;
  std::string scalar;

  ScanResult r;

  params.leadingSpaces = false;

  while (INPUT) {
    // ********************************
    // Phase #1: scan until line ending

    ScanLine(INPUT, params, scalar, r);
    pastOpeningBreak |= r.foundNonEmptyLine;

    // eof? if we're looking to eat something, then we throw
    if (!INPUT) {
      if (params.eatEnd) {
        throw ParserException(INPUT.mark(), ErrorMsg::EOF_IN_SCALAR);
      }
      break;
    }

    // doc indicator?
    if (params.onDocIndicator == BREAK && INPUT.column() == 0) {
      if (MatchDocIndicator(INPUT)) {
        break;
      }
    }

    // are we done via character match?
    if (r.endMatch >= 0) {
      if (params.eatEnd) {
        INPUT.eat(r.endMatch);
      }
      break;
    }

    // do we remove trailing whitespace?
    if (params.fold == FOLD_FLOW) {
      if (r.lastNonWhitespaceChar < scalar.size()) {
        scalar.erase(r.lastNonWhitespaceChar);
      }
    }
    // ********************************
    // Phase #2: eat line ending
    INPUT.EatLineBreak();

    // ********************************
    // Phase #3: scan initial spaces

    EatToIndentation(INPUT, params, !r.foundNonEmptyLine);

    // update indent if we're auto-detecting
    if (params.detectIndent && !r.foundNonEmptyLine) {
      params.indent = std::max(params.indent, INPUT.column());
    }

    // and then the rest of the whitespace
    if (INPUT.peek() == ' ' || INPUT.peek() == '\t') {
      EatAfterIndentation(INPUT, params);
    }

    // was this an empty line?
    Exp::Source<4> input;
    INPUT.LookaheadBuffer(input);
    bool nextEmptyLine = Exp::Break::Matches(input);
    bool nextMoreIndented = Exp::Blank::Matches(input);
    if (params.fold == FOLD_BLOCK && foldedNewlineCount == 0 && nextEmptyLine)
      foldedNewlineStartedMoreIndented = moreIndented;

    if (pastOpeningBreak) {
      foldedNewlineCount = HandleFolding(scalar, params, INPUT.column(),
                                         r.escapedNewline,
                                         emptyLine, moreIndented,
                                         nextEmptyLine, nextMoreIndented,
                                         r.foundNonEmptyLine,
                                         foldedNewlineStartedMoreIndented,
                                         foldedNewlineCount);
    }

    emptyLine = nextEmptyLine;
    moreIndented = nextMoreIndented;
    pastOpeningBreak = true;

    // are we done via indentation?
    if (!emptyLine && INPUT.column() < params.indent) {
      params.leadingSpaces = true;
      break;
    }
  }

  PostProcess(scalar, params, r.lastEscapedChar);

  return scalar;
}


TEST_NO_INLINE
static void ScanLine(Stream& INPUT, const ScanScalarParams& params,
                           std::string& scalar, ScanResult& out) {

  const size_t bufferSize = 256;
  char buffer[bufferSize];
  size_t bufferFill = 0;
  size_t scalarLength = scalar.length();

  out.lastNonWhitespaceChar = scalarLength;
  out.escapedNewline = false;

  while (INPUT) {

    Exp::Source<4> input;
    INPUT.LookaheadBuffer(input);

    bool isWhiteSpace = Exp::Blank::Matches(input);

    if (!isWhiteSpace) {
      if (Exp::Break::Matches(input)) { break; }

      // document indicator?
      if (unlikely(INPUT.column() == 0) &&
          MatchDocIndicator(INPUT)) {
        if (params.onDocIndicator == BREAK) {
          break;
        } else if (params.onDocIndicator == THROW) {
          throw ParserException(INPUT.mark(), ErrorMsg::DOC_IN_SCALAR);
        }
      }
    }

    // keep end posiion
    if ((out.endMatch = params.end(input)) >= 0) {
      break;
    }

    out.foundNonEmptyLine = true;

    if (likely(params.escape != input[0])) {
      // just add the character
      if (unlikely(bufferFill == bufferSize)) {
        scalar.insert(scalar.size(), buffer, bufferSize);
        bufferFill = 0;
      }

      buffer[bufferFill++] = input[0];

      scalarLength++;

      INPUT.eat();

      if (!isWhiteSpace) {
        out.lastNonWhitespaceChar = scalarLength;
      }

    } else {
      // escaped newline? (only if we're escaping on slash)
      if (params.escape == '\\' && Exp::EscBreak::Matches(input)) {
        // eat escape character and get out (but preserve trailing whitespace!)
        INPUT.eat();
        out.lastEscapedChar = out.lastNonWhitespaceChar = scalarLength;
        out.escapedNewline = true;
        break;

      } else {
        if (bufferFill > 0) {
          scalar.insert(scalar.size(), buffer, bufferFill);
          bufferFill = 0;
        }

        scalar += Exp::Escape(INPUT);
        scalarLength = scalar.size();

        out.lastEscapedChar = out.lastNonWhitespaceChar = scalarLength;
      }
    }
  }

  if (bufferFill > 0) {
    scalar.insert(scalar.size(), buffer, bufferFill);
  }
}

TEST_NO_INLINE
static void EatToIndentation(Stream& INPUT, ScanScalarParams& params, bool foundEmptyLine) {

  using namespace Exp;

  using _ = Char<' '>;

  using SpaceInvaders = Matcher<Count<_,_,_,_,_,_,_,_>>;

  // first the required indentation
  // This can be:
  // - BlockScalar (detectIndent/colum<indent)
  // - PlainScalar (colum<indent) /
  int max = params.indent - INPUT.column();
  if (params.detectIndent && foundEmptyLine) {
    max = std::numeric_limits<int>::max();
  }

  // Don't eat the whitespace before comments
  while (max > 0) {

    auto input = INPUT.GetLookaheadBuffer(8);

    int pos = SpaceInvaders::Match(input);

    // No, nothing to eat!
    if (pos == 0) { break; }

    // Pos can be up to 8. Dont eat before potential comment.
    if (params.indentFn && (pos == 8 || input[pos] == '#')) {
      pos -= 1;
    }
    if (max < pos) { pos = max; }

    // Eat spaces
    for (int i = 0; i < pos; i++) {
      INPUT.eat();
    }

    if (pos < 7 || input[7] != ' ') {
      break;
    }
    max -= pos;
  }
}

TEST_NO_INLINE
static void EatAfterIndentation(Stream& INPUT, ScanScalarParams& params) {

  for (char c = INPUT.peek(); (c == ' ' || c == '\t'); c = INPUT.peek()) {
    // we check for tabs that masquerade as indentation
    if (c == '\t' && INPUT.column() < params.indent &&
        params.onTabInIndentation == THROW) {
      throw ParserException(INPUT.mark(), ErrorMsg::TAB_IN_INDENTATION);
    }

    if (!params.eatLeadingWhitespace) {
      break;
    }
    // FIXME: 2
    Exp::Source<4> input;
    INPUT.LookaheadBuffer(input);
    if (params.indentFn && params.indentFn(input) >= 0) {
      break;
    }
    //printf("check 2\n");

    INPUT.eat();
  }
}

TEST_NO_INLINE
static int HandleFolding(std::string& scalar,
                         const ScanScalarParams& params,
                         int column, bool escapedNewline,
                         bool emptyLine, bool moreIndented,
                         bool nextEmptyLine, bool nextMoreIndented,
                         bool foundNonEmptyLine,
                         bool foldedNewlineStartedMoreIndented,
                         int foldedNewlineCount) {

  // for block scalars, we always start with a newline, so we should ignore it
  // (not fold or keep)
  switch (params.fold) {
    case DONT_FOLD:
      scalar += '\n';
      break;
    case FOLD_BLOCK:
      if (!emptyLine && !nextEmptyLine && !moreIndented &&
          !nextMoreIndented && column >= params.indent) {
        scalar += ' ';
      } else if (nextEmptyLine) {
        foldedNewlineCount++;
      } else {
        scalar += '\n';
      }

      if (!nextEmptyLine && foldedNewlineCount > 0) {
        scalar += std::string(foldedNewlineCount - 1, '\n');
        if (foldedNewlineStartedMoreIndented ||
            nextMoreIndented | !foundNonEmptyLine) {
          scalar += '\n';
        }
        foldedNewlineCount = 0;
      }
      break;
    case FOLD_FLOW:
      if (nextEmptyLine) {
        scalar += '\n';
      } else if (!emptyLine && !escapedNewline) {
        scalar += ' ';
      }
      break;
  }
  return foldedNewlineCount;
}

TEST_NO_INLINE
static void PostProcess(std::string& scalar, ScanScalarParams& params, size_t lastEscapedChar) {
  // post-processing
  if (params.trimTrailingSpaces) {
    std::size_t pos = scalar.size()-1;
    while (pos != std::string::npos && scalar[pos] == ' ') { pos--; }
    //std::size_t pos = scalar.find_last_not_of(' ');

    if (lastEscapedChar != std::string::npos) {
      if (pos < lastEscapedChar || pos == std::string::npos) {
        pos = lastEscapedChar;
      }
    }
    if (pos < scalar.size()) {
      scalar.erase(pos + 1);
    }
  }

  switch (params.chomp) {
    case CLIP: {
      std::size_t pos = scalar.find_last_not_of('\n');
      if (lastEscapedChar != std::string::npos) {
        if (pos < lastEscapedChar || pos == std::string::npos) {
          pos = lastEscapedChar;
        }
      }
      if (pos == std::string::npos) {
        scalar.erase();
      } else if (pos + 1 < scalar.size()) {
        scalar.erase(pos + 2);
      }
    } break;
    case STRIP: {
      std::size_t pos = scalar.find_last_not_of('\n');
      if (lastEscapedChar != std::string::npos) {
        if (pos < lastEscapedChar || pos == std::string::npos) {
          pos = lastEscapedChar;
        }
      }
      if (pos == std::string::npos) {
        scalar.erase();
      } else if (pos < scalar.size()) {
        scalar.erase(pos + 1);
      }
    } break;
    default:
      break;
  }
}

}

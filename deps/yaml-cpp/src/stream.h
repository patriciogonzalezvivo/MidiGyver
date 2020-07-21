#pragma once

#include "yaml-cpp/noncopyable.h"
#include "yaml-cpp/mark.h"
#include "streamcharsource.h"

#include <cstddef>
#include <deque>
#include <vector>
#include <ios>
#include <iostream>
#include <set>
#include <string>
#include <array>
#include <cstring>
#include <cassert>

namespace YAML {
class Stream : private noncopyable {
 public:
  friend class StreamCharSource;

  explicit Stream(std::istream& input);
  ~Stream();

  operator bool() const {
    return m_char != Stream::eof();
  }

  bool operator!() const { return !static_cast<bool>(*this); }

  char peek() const {
    return m_char;
  }

  char get();
  std::string get(int n);
  void eat(int n);
  // NB: Do not use to eat line breaks! Use eat(n) instead.
  void eat() {
    m_readaheadPos++;
    m_mark.pos++;

    assert(m_char != '\n');
    m_mark.column++;

    if (ReadAheadTo(0)) {
      m_char = m_buffer[m_readaheadPos];
    } else {
      m_char = Stream::eof();
    }
  }

  static constexpr char eof() { return 0x04; }

  const Mark mark() const { return m_mark; }
  int pos() const { return m_mark.pos; }
  int line() const { return m_mark.line; }
  int column() const { return m_mark.column; }
  void ResetColumn() { m_mark.column = 0; }
  void EatSpace();
  void EatToEndOfLine();
  void EatBlanks();
  bool EatLineBreak();

  // Must be large enough for all regexp we use
  static constexpr size_t lookahead_elements = 8;

  void LookaheadBuffer(Exp::Source<1>& out) const {
    out[0] = m_char;
  }

  void LookaheadBuffer(Exp::Source<2>& out) const {
    int offset = m_mark.pos - m_lookahead.streamPos;
    if (m_lookahead.available > 2 + offset) {
      out[0] = m_lookahead.buffer[offset];
      out[1] = m_lookahead.buffer[offset+1];
      return;
    }
    m_lookahead.streamPos += offset;
    UpdateLookahead();

    out[0] = m_lookahead.buffer[0];
    out[1] = m_lookahead.buffer[1];
  }

  void LookaheadBuffer(Exp::Source<4>& out) const {
    int offset = m_mark.pos - m_lookahead.streamPos;
    auto dst = reinterpret_cast<uint32_t*>(out.data());

    if (__builtin_expect(m_lookahead.available > 4 + offset, 1)) {
      auto src = reinterpret_cast<uint64_t*>(m_lookahead.buffer.data());
      dst[0] = src[0] >> (offset * 8);
    } else {
      auto src = reinterpret_cast<uint32_t*>(m_lookahead.buffer.data());
      m_lookahead.streamPos += offset;
      UpdateLookahead();
      dst[0] = src[0];
    }
  }

  const Exp::StreamSource& GetLookaheadBuffer(int lookahead) const {
    int offset = m_mark.pos - m_lookahead.streamPos;
    if (offset == 0 && m_lookahead.available >= lookahead) {
      return m_lookahead.buffer;
    }
    m_lookahead.streamPos += offset;

    if (m_lookahead.available > lookahead + offset) {
      m_lookahead.available -= offset;

      uint64_t* buf = reinterpret_cast<uint64_t*>(m_lookahead.buffer.data());
      buf[0] >>= (8 * offset);
    } else {
      UpdateLookahead();
    }
    return m_lookahead.buffer;
  }

 private:

  enum CharacterSet { utf8, utf16le, utf16be, utf32le, utf32be };

  mutable struct Lookahead {
    int streamPos = 0;
    int available = 0;
    Exp::StreamSource buffer;
  } m_lookahead;

  Mark m_mark;
  char m_char = Stream::eof();

  mutable size_t m_readaheadPos = 0;
  mutable size_t m_readaheadSize = 0;
  mutable std::vector<char> m_readahead;

  mutable const char* m_buffer;

  std::istream& m_input;
  CharacterSet m_charSet;

  unsigned char* const m_pPrefetched;
  mutable size_t m_nPrefetchedAvailable;
  mutable size_t m_nPrefetchedUsed;

  inline void AdvanceCurrent();
  bool ReadAheadTo(size_t i) const;
  bool _ReadAheadTo(size_t i) const;
  void StreamInUtf8() const;
  void StreamInUtf16() const;
  void StreamInUtf32() const;
  unsigned char GetNextByte() const;

  void QueueUnicodeCodepoint(unsigned long ch) const;

  void UpdateLookahead() const;

  static CharacterSet determineCharachterSet(std::istream& input, int& skip);

};

inline bool Stream::ReadAheadTo(size_t i) const {
  if (m_readaheadSize - m_readaheadPos > i) {
    return true;
  }
  return _ReadAheadTo(i);
}
}

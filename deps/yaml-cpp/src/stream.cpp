#include <iostream>

#include "stream.h"
#include "streamcharsource.h"
#include "exp.h"

#include <sstream>

#ifndef YAML_PREFETCH_SIZE
#define YAML_PREFETCH_SIZE 8192
//#define YAML_PREFETCH_SIZE 1024
#endif

#define S_ARRAY_SIZE(A) (sizeof(A) / sizeof(*(A)))
#define S_ARRAY_END(A) ((A) + S_ARRAY_SIZE(A))

#define likely(x)       __builtin_expect(!!(x), 1)

#define CP_REPLACEMENT_CHARACTER (0xFFFD)

namespace YAML {
enum UtfIntroState {
  uis_start,
  uis_utfbe_b1,
  uis_utf32be_b2,
  uis_utf32be_bom3,
  uis_utf32be,
  uis_utf16be,
  uis_utf16be_bom1,
  uis_utfle_bom1,
  uis_utf16le_bom2,
  uis_utf32le_bom3,
  uis_utf16le,
  uis_utf32le,
  uis_utf8_imp,
  uis_utf16le_imp,
  uis_utf32le_imp3,
  uis_utf8_bom1,
  uis_utf8_bom2,
  uis_utf8,
  uis_error
};

enum UtfIntroCharType {
  uict00,
  uictBB,
  uictBF,
  uictEF,
  uictFE,
  uictFF,
  uictAscii,
  uictOther,
  uictMax
};

static bool s_introFinalState[] = {
    false,  // uis_start
    false,  // uis_utfbe_b1
    false,  // uis_utf32be_b2
    false,  // uis_utf32be_bom3
    true,   // uis_utf32be
    true,   // uis_utf16be
    false,  // uis_utf16be_bom1
    false,  // uis_utfle_bom1
    false,  // uis_utf16le_bom2
    false,  // uis_utf32le_bom3
    true,   // uis_utf16le
    true,   // uis_utf32le
    false,  // uis_utf8_imp
    false,  // uis_utf16le_imp
    false,  // uis_utf32le_imp3
    false,  // uis_utf8_bom1
    false,  // uis_utf8_bom2
    true,   // uis_utf8
    true,   // uis_error
};

static UtfIntroState s_introTransitions[][uictMax] = {
    // uict00,           uictBB,           uictBF,           uictEF,
    // uictFE,           uictFF,           uictAscii,        uictOther
    {uis_utfbe_b1, uis_utf8, uis_utf8, uis_utf8_bom1, uis_utf16be_bom1,
     uis_utfle_bom1, uis_utf8_imp, uis_utf8},
    {uis_utf32be_b2, uis_utf8, uis_utf8, uis_utf8, uis_utf8, uis_utf8,
     uis_utf16be, uis_utf8},
    {uis_utf32be, uis_utf8, uis_utf8, uis_utf8, uis_utf32be_bom3, uis_utf8,
     uis_utf8, uis_utf8},
    {uis_utf8, uis_utf8, uis_utf8, uis_utf8, uis_utf8, uis_utf32be, uis_utf8,
     uis_utf8},
    {uis_utf32be, uis_utf32be, uis_utf32be, uis_utf32be, uis_utf32be,
     uis_utf32be, uis_utf32be, uis_utf32be},
    {uis_utf16be, uis_utf16be, uis_utf16be, uis_utf16be, uis_utf16be,
     uis_utf16be, uis_utf16be, uis_utf16be},
    {uis_utf8, uis_utf8, uis_utf8, uis_utf8, uis_utf8, uis_utf16be, uis_utf8,
     uis_utf8},
    {uis_utf8, uis_utf8, uis_utf8, uis_utf8, uis_utf16le_bom2, uis_utf8,
     uis_utf8, uis_utf8},
    {uis_utf32le_bom3, uis_utf16le, uis_utf16le, uis_utf16le, uis_utf16le,
     uis_utf16le, uis_utf16le, uis_utf16le},
    {uis_utf32le, uis_utf16le, uis_utf16le, uis_utf16le, uis_utf16le,
     uis_utf16le, uis_utf16le, uis_utf16le},
    {uis_utf16le, uis_utf16le, uis_utf16le, uis_utf16le, uis_utf16le,
     uis_utf16le, uis_utf16le, uis_utf16le},
    {uis_utf32le, uis_utf32le, uis_utf32le, uis_utf32le, uis_utf32le,
     uis_utf32le, uis_utf32le, uis_utf32le},
    {uis_utf16le_imp, uis_utf8, uis_utf8, uis_utf8, uis_utf8, uis_utf8,
     uis_utf8, uis_utf8},
    {uis_utf32le_imp3, uis_utf16le, uis_utf16le, uis_utf16le, uis_utf16le,
     uis_utf16le, uis_utf16le, uis_utf16le},
    {uis_utf32le, uis_utf16le, uis_utf16le, uis_utf16le, uis_utf16le,
     uis_utf16le, uis_utf16le, uis_utf16le},
    {uis_utf8, uis_utf8_bom2, uis_utf8, uis_utf8, uis_utf8, uis_utf8, uis_utf8,
     uis_utf8},
    {uis_utf8, uis_utf8, uis_utf8, uis_utf8, uis_utf8, uis_utf8, uis_utf8,
     uis_utf8},
    {uis_utf8, uis_utf8, uis_utf8, uis_utf8, uis_utf8, uis_utf8, uis_utf8,
     uis_utf8},
};

static char s_introUngetCount[][uictMax] = {
    // uict00, uictBB, uictBF, uictEF, uictFE, uictFF, uictAscii, uictOther
    {0, 1, 1, 0, 0, 0, 0, 1},
    {0, 2, 2, 2, 2, 2, 2, 2},
    {3, 3, 3, 3, 0, 3, 3, 3},
    {4, 4, 4, 4, 4, 0, 4, 4},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {2, 2, 2, 2, 2, 0, 2, 2},
    {2, 2, 2, 2, 0, 2, 2, 2},
    {0, 1, 1, 1, 1, 1, 1, 1},
    {0, 2, 2, 2, 2, 2, 2, 2},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {0, 2, 2, 2, 2, 2, 2, 2},
    {0, 3, 3, 3, 3, 3, 3, 3},
    {4, 4, 4, 4, 4, 4, 4, 4},
    {2, 0, 2, 2, 2, 2, 2, 2},
    {3, 3, 0, 3, 3, 3, 3, 3},
    {1, 1, 1, 1, 1, 1, 1, 1},
};

inline UtfIntroCharType IntroCharTypeOf(std::istream::int_type ch) {
  if (std::istream::traits_type::eof() == ch) {
    return uictOther;
  }

  switch (ch) {
    case 0:
      return uict00;
    case 0xBB:
      return uictBB;
    case 0xBF:
      return uictBF;
    case 0xEF:
      return uictEF;
    case 0xFE:
      return uictFE;
    case 0xFF:
      return uictFF;
  }

  if ((ch > 0) && (ch < 0xFF)) {
    return uictAscii;
  }

  return uictOther;
}

inline char Utf8Adjust(unsigned long ch, unsigned char lead_bits,
                       unsigned char rshift) {
  const unsigned char header = ((1 << lead_bits) - 1) << (8 - lead_bits);
  const unsigned char mask = (0xFF >> (lead_bits + 1));
  return static_cast<char>(
      static_cast<unsigned char>(header | ((ch >> rshift) & mask)));
}

inline void Stream::QueueUnicodeCodepoint(unsigned long ch) const {
  // We are not allowed to queue the Stream::eof() codepoint, so
  // replace it with CP_REPLACEMENT_CHARACTER

  auto& q = m_readahead;

  if (static_cast<unsigned long>(Stream::eof()) == ch) {
    ch = CP_REPLACEMENT_CHARACTER;
  }

  if (ch < 0x80) {
    q.push_back(Utf8Adjust(ch, 0, 0));
  } else if (ch < 0x800) {
    q.push_back(Utf8Adjust(ch, 2, 6));
    q.push_back(Utf8Adjust(ch, 1, 0));
  } else if (ch < 0x10000) {
    q.push_back(Utf8Adjust(ch, 3, 12));
    q.push_back(Utf8Adjust(ch, 1, 6));
    q.push_back(Utf8Adjust(ch, 1, 0));
  } else {
    q.push_back(Utf8Adjust(ch, 4, 18));
    q.push_back(Utf8Adjust(ch, 1, 12));
    q.push_back(Utf8Adjust(ch, 1, 6));
    q.push_back(Utf8Adjust(ch, 1, 0));
  }
}


Stream::CharacterSet Stream::determineCharachterSet(std::istream& input, int& skip) {
  // Determine (or guess) the character-set by reading the BOM, if any.  See
  // the YAML specification for the determination algorithm.
  typedef std::istream::traits_type char_traits;
  char_traits::int_type intro[4];
  int nIntroUsed = 0;
  UtfIntroState state = uis_start;
  for (; !s_introFinalState[state];) {
    std::istream::int_type ch = input.get();
    skip++;
    intro[nIntroUsed++] = ch;
    UtfIntroCharType charType = IntroCharTypeOf(ch);
    UtfIntroState newState = s_introTransitions[state][charType];
    int nUngets = s_introUngetCount[state][charType];
    if (nUngets > 0) {
      skip -= nUngets;

      input.clear();
      for (; nUngets > 0; --nUngets) {
        if (char_traits::eof() != intro[--nIntroUsed])
          input.putback(char_traits::to_char_type(intro[nIntroUsed]));
      }
    }
    state = newState;
  }

  switch (state) {
    case uis_utf8:
      return utf8;
    case uis_utf16le:
      return utf16le;
    case uis_utf16be:
      return utf16be;
    case uis_utf32le:
      return utf32le;
    case uis_utf32be:
      return utf32be;
    default:
      break;
  }
  return utf8;
}

Stream::Stream(std::istream& input)
    : m_input(input),
      m_pPrefetched(new unsigned char[YAML_PREFETCH_SIZE]),
      m_nPrefetchedAvailable(0),
      m_nPrefetchedUsed(0) {

  if (!input)
    return;

  int skip;
  m_charSet = determineCharachterSet(input, skip);

  ReadAheadTo(0);

  if (m_readaheadSize > 0) {
    m_char = m_buffer[0];
  } else {
    m_char = Stream::eof();
  }
}

Stream::~Stream() {
    if (m_pPrefetched) {
        delete[] m_pPrefetched;
    }
}

// get
// . Extracts a character from the stream and updates our position
char Stream::get() {
  char ch = peek();
  AdvanceCurrent();

  return ch;
}

// get
// . Extracts 'n' characters from the stream and updates our position
std::string Stream::get(int n) {
  std::string ret;
  ret.reserve(n);
  for (int i = 0; i < n; i++)
    ret += get();

  return ret;
}

// eat
// . Eats 'n' characters and updates our position.
void Stream::eat(int n) {
  for (int i = 0; i < n; i++) {
    AdvanceCurrent();
  }
}

void Stream::AdvanceCurrent() {

    m_readaheadPos++;
    m_mark.pos++;

   // FIXME - what about escaped newlines?
   if (likely(m_char != '\n')) {
       m_mark.column++;
   } else {
       m_mark.column = 0;
       m_mark.line++;
   }

   if (likely(ReadAheadTo(0))) {
       m_char = m_buffer[m_readaheadPos];
   } else {
       m_char = Stream::eof();
   }
}

void Stream::EatSpace() {
  if (m_char != ' ') { return; }

  int pos = m_readaheadPos;
  int available = m_readaheadSize;

  char ch;
  do {
    if (++pos == available) {
      int count = pos - m_readaheadPos;
      m_readaheadPos = pos;

      m_mark.pos += count;
      m_mark.column += count;

      if (!ReadAheadTo(0)) {
        m_char = Stream::eof();
        return;
      }
      pos = m_readaheadPos;
      available = m_readaheadSize;
    }

    ch = m_buffer[pos];

  } while (ch == ' ');

  int count = pos - m_readaheadPos;
  m_readaheadPos = pos;

  m_mark.pos += count;
  m_mark.column += count;

  if (!ReadAheadTo(0)) {
    m_char = Stream::eof();
    return;
  }

  m_char = m_buffer[m_readaheadPos];
}

bool Stream::EatLineBreak() {

  if (m_char == '\n') {
    m_readaheadPos++;
    m_mark.pos++;
    m_mark.column = 0;
    m_mark.line++;
  } else if (m_char == '\r' &&
             (ReadAheadTo(1) &&
              m_buffer[m_readaheadPos + 1] == '\n')) {
    m_readaheadPos += 2;
    m_mark.pos += 2;
    m_mark.column = 0;
    m_mark.line++;
  } else {
    return false;
  }

  if (ReadAheadTo(0)) {
    m_char = m_buffer[m_readaheadPos];
  } else {
    m_char = Stream::eof();
  }
  return true;
}

void Stream::EatToEndOfLine() {

  while (m_char != '\n' && m_char != Stream::eof()) {

    m_readaheadPos++;
    m_mark.pos++;
    m_mark.column++;

    if (ReadAheadTo(0)) {
      m_char = m_buffer[m_readaheadPos];
    } else {
      m_char = Stream::eof();
    }
  }
}

void Stream::EatBlanks() {

  while (m_char == ' ' || m_char == '\t') {

    m_readaheadPos++;
    m_mark.pos++;
    m_mark.column++;

    if (ReadAheadTo(0)) {
      m_char = m_buffer[m_readaheadPos];
    } else {
      m_char = Stream::eof();
    }
  }
}

void Stream::UpdateLookahead() const {

  const size_t want = lookahead_elements;

  if (m_readaheadPos + want > m_readaheadSize) {
    _ReadAheadTo(want);
  }

  if (likely(m_readaheadPos + want * 2 < m_readaheadSize)) {

    const char* src = reinterpret_cast<const char*>(m_buffer + m_readaheadPos);

    // 8 byte aligned source
    const uint64_t* aligned = reinterpret_cast<const uint64_t*>(
      (reinterpret_cast<size_t>(src) + 7) & ~7);

    // Always 8 byte aligned
    uint64_t* dst = reinterpret_cast<uint64_t*>(m_lookahead.buffer.data());

    size_t offset = reinterpret_cast<const char*>(aligned) - src;

    if (offset != 0) {
      // fill with rest of previous 8 bytes
      dst[0] = aligned[-1] >> (8 * (want - offset));
      // fill remaining space from current src offset
      dst[0] |= aligned[0] << (8 * offset);
    } else {
      dst[0] = aligned[0];
    }
    m_lookahead.available = want;

  } else {
    size_t max = std::min(m_readaheadSize - m_readaheadPos, want);

    for (size_t i = 0; i < max; i++) {
      m_lookahead.buffer[i] = m_buffer[m_readaheadPos + i];
    }
    if (max < want) {
      m_lookahead.buffer[max] = Stream::eof();
      // added the EOF
      max += 1;
    }

    m_lookahead.available = max;
  }
}

bool Stream::_ReadAheadTo(size_t i) const {
    if (!m_input) { return false; }

    const size_t readaheadLimit = 32;
    if (m_readaheadPos > readaheadLimit + i) {
        m_readahead.erase(m_readahead.begin(), m_readahead.begin() + readaheadLimit);
        m_readaheadPos -= readaheadLimit;
    }

    while (m_input.good() && (m_readahead.size()  - m_readaheadPos <= i)) {
    switch (m_charSet) {
      case utf8:
        StreamInUtf8();
        break;
      case utf16le:
        StreamInUtf16();
        break;
      case utf16be:
        StreamInUtf16();
        break;
      case utf32le:
        StreamInUtf32();
        break;
      case utf32be:
        StreamInUtf32();
        break;
    }
  }

  // signal end of stream
  if (!m_input.good())
    m_readahead.push_back(Stream::eof());

  m_readaheadSize = m_readahead.size();
  m_buffer = m_readahead.data();

  return m_readahead.size() - m_readaheadPos > i;
}

void Stream::StreamInUtf8() const {
  unsigned char b = GetNextByte();
  if (m_input.good()) {
    m_readahead.push_back(b);
  }
}

void Stream::StreamInUtf16() const {
  unsigned long ch = 0;
  unsigned char bytes[2];
  int nBigEnd = (m_charSet == utf16be) ? 0 : 1;

  bytes[0] = GetNextByte();
  bytes[1] = GetNextByte();
  if (!m_input.good()) {
    return;
  }
  ch = (static_cast<unsigned long>(bytes[nBigEnd]) << 8) |
       static_cast<unsigned long>(bytes[1 ^ nBigEnd]);

  if (ch >= 0xDC00 && ch < 0xE000) {
    // Trailing (low) surrogate...ugh, wrong order
    QueueUnicodeCodepoint(CP_REPLACEMENT_CHARACTER);
    return;
  } else if (ch >= 0xD800 && ch < 0xDC00) {
    // ch is a leading (high) surrogate

    // Four byte UTF-8 code point

    // Read the trailing (low) surrogate
    for (;;) {
      bytes[0] = GetNextByte();
      bytes[1] = GetNextByte();
      if (!m_input.good()) {
        QueueUnicodeCodepoint(CP_REPLACEMENT_CHARACTER);
        return;
      }
      unsigned long chLow = (static_cast<unsigned long>(bytes[nBigEnd]) << 8) |
                            static_cast<unsigned long>(bytes[1 ^ nBigEnd]);
      if (chLow < 0xDC00 || chLow >= 0xE000) {
        // Trouble...not a low surrogate.  Dump a REPLACEMENT CHARACTER into the
        // stream.
        QueueUnicodeCodepoint(CP_REPLACEMENT_CHARACTER);

        // Deal with the next UTF-16 unit
        if (chLow < 0xD800 || chLow >= 0xE000) {
          // Easiest case: queue the codepoint and return
          QueueUnicodeCodepoint(ch);
          return;
        } else {
          // Start the loop over with the new high surrogate
          ch = chLow;
          continue;
        }
      }

      // Select the payload bits from the high surrogate
      ch &= 0x3FF;
      ch <<= 10;

      // Include bits from low surrogate
      ch |= (chLow & 0x3FF);

      // Add the surrogacy offset
      ch += 0x10000;
      break;
    }
  }

  QueueUnicodeCodepoint(ch);
}

inline char* ReadBuffer(unsigned char* pBuffer) {
  return reinterpret_cast<char*>(pBuffer);
}

unsigned char Stream::GetNextByte() const {
  if (m_nPrefetchedUsed >= m_nPrefetchedAvailable) {
    std::streambuf* pBuf = m_input.rdbuf();

    m_nPrefetchedAvailable = static_cast<std::size_t>(pBuf->sgetn(ReadBuffer(m_pPrefetched), YAML_PREFETCH_SIZE));

    m_nPrefetchedUsed = 0;
    if (!m_nPrefetchedAvailable) {
      m_input.setstate(std::ios_base::eofbit);
    }

    if (0 == m_nPrefetchedAvailable) {
      return 0;
    }
  }

  return m_pPrefetched[m_nPrefetchedUsed++];
}

void Stream::StreamInUtf32() const {
  static int indexes[2][4] = {{3, 2, 1, 0}, {0, 1, 2, 3}};

  unsigned long ch = 0;
  unsigned char bytes[4];
  int* pIndexes = (m_charSet == utf32be) ? indexes[1] : indexes[0];

  bytes[0] = GetNextByte();
  bytes[1] = GetNextByte();
  bytes[2] = GetNextByte();
  bytes[3] = GetNextByte();
  if (!m_input.good()) {
    return;
  }

  for (int i = 0; i < 4; ++i) {
    ch <<= 8;
    ch |= bytes[pIndexes[i]];
  }

  QueueUnicodeCodepoint(ch);
}
}

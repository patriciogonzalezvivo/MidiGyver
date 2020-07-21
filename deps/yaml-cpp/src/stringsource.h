#pragma once

#include <cstddef>

namespace YAML {
class StringCharSource {
 public:
  StringCharSource(const char* str, std::size_t size)
      : m_str(str), m_size(size), m_offset(0) {}

  operator bool() const { return m_offset < m_size; }
  char operator[](std::size_t i) const {
    return (m_offset + i < m_size) ? m_str[m_offset + i] : 0x04; // EOF
  }
  bool operator!() const { return !static_cast<bool>(*this); }

  const StringCharSource operator+(int i) const {
    StringCharSource source(*this);
    if (static_cast<int>(source.m_offset) + i >= 0)
      source.m_offset += i;
    else
      source.m_offset = 0;
    return source;
  }

  StringCharSource& operator++() {
    ++m_offset;
    return *this;
  }

  StringCharSource& operator+=(std::size_t offset) {
    m_offset += offset;
    return *this;
  }

  char get() const { return m_str[m_offset]; }

 private:
  const char* m_str;
  std::size_t m_size;
  std::size_t m_offset;
};
}

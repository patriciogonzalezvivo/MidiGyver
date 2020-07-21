#pragma once

#include "yaml-cpp/dll.h"

namespace YAML {
namespace detail {

template <typename T, bool owned_region>
struct ref_holder {

   using holder = ref_holder<T, owned_region>;

   __attribute__((always_inline))
  ~ref_holder() { release(); }

  ref_holder(T* ptr) {
    if (ptr) {
      ptr->hold();
    }
    m_ptr = ptr;
  }

  ref_holder(const holder& ref) {
    if (ref.m_ptr) {
      ref.m_ptr->hold();
    }
    m_ptr = ref.m_ptr;
  }

  ref_holder(holder&& ref) {
    m_ptr = ref.m_ptr;
    ref.m_ptr = nullptr;
  }

  holder& operator=(const holder& ref) {
    if (ref.m_ptr == m_ptr) {
      return *this;
    }
    if (ref.m_ptr) {
      ref.m_ptr->hold();
    }
    release();

    m_ptr = ref.m_ptr;
    return *this;
  }

  holder& operator=(holder&& ref) {
    if (ref.m_ptr == m_ptr) {
      return *this;
    }
    release();

    m_ptr = ref.m_ptr;
    ref.m_ptr = nullptr;
    return *this;
  }

  bool operator==(const holder& ref) const { return m_ptr == ref.m_ptr; }
  bool operator!=(const holder& ref) const { return m_ptr != ref.m_ptr; }

  const T* operator->() const { return m_ptr; }
  T* operator->() { return m_ptr; }

  const T& operator*() const { return *m_ptr; }
  T& operator*() { return *m_ptr; }

  const T* get() { return m_ptr; }

  void reset(T* ptr) {
    if (ptr == m_ptr) {
      return;
    }
    if (ptr) {
      ptr->hold();
    }
    release();

    m_ptr = ptr;
  }

  operator bool() const { return m_ptr != nullptr; }

 private:
  template<bool D = owned_region, typename std::enable_if<D, int>::type = 0>
  void release() {
    if (m_ptr && m_ptr->release()) {
      delete m_ptr;
      m_ptr = nullptr;
    }
  }
  template<bool D = owned_region, typename std::enable_if<!D, int>::type = 0>
  void release() {
    if (m_ptr && m_ptr->release()) {
      m_ptr->~T();
      m_ptr = nullptr;
    }
  }

  T* m_ptr;
};

struct ref_counted {

  void hold() { m_refs++; }
  bool release() { return (--m_refs == 0); }

 private:
  uint32_t m_refs = 0;
};
}
}

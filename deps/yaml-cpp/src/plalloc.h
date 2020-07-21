// This is free and unencumbered software released into the public domain.

// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.

// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// For more information, please refer to <http://unlicense.org/>
//
// https://probablydance.com/2014/11/09/plalloc-a-simple-stateful-
//         allocator-for-node-based-containers/
//

#pragma once

#include <memory>
#include <vector>

template<typename T, std::size_t N>
struct plalloc {

  typedef T value_type;

  plalloc() = default;
  template<typename U>
  plalloc(const plalloc<U, N> &) {}
  plalloc(const plalloc &) {}
  plalloc & operator=(const plalloc &) { return *this; }
  plalloc(plalloc &&) = default;
  plalloc & operator=(plalloc &&) = default;

  typedef std::true_type propagate_on_container_copy_assignment;
  typedef std::true_type propagate_on_container_move_assignment;
  typedef std::true_type propagate_on_container_swap;

  bool operator==(const plalloc & other) const {
    return this == &other;
  }
  bool operator!=(const plalloc & other) const {
    return !(*this == other);
  }

  T * allocate(size_t num_to_allocate) {
    if (num_to_allocate != 1) {
      return static_cast<T *>(::operator new(sizeof(T) * num_to_allocate));

    } else if (available.empty()) {
      // first allocate N, then double whenever
      // we run out of memory
      size_t to_allocate = N << memory.size();
      //printf("alloc %lu\n", to_allocate);
      available.reserve(to_allocate);
      std::unique_ptr<value_holder[]> allocated(new value_holder[to_allocate]);
      value_holder * first_new = allocated.get();
      memory.emplace_back(std::move(allocated));
      size_t to_return = to_allocate - 1;
      for (size_t i = 0; i < to_return; ++i) {
        available.push_back(std::addressof(first_new[i].value));
      }
      return std::addressof(first_new[to_return].value);

    } else {
      T * result = available.back();
      available.pop_back();
      return result;
    }
  }
  void deallocate(T * ptr, size_t num_to_free) {
    if (num_to_free == 1) {
      available.push_back(ptr);
    } else {
      ::operator delete(ptr);
    }
  }

  // boilerplate that shouldn't be needed, except
  // libstdc++ doesn't use allocator_traits yet
  template<typename U>
  struct rebind {
    typedef plalloc<U, N> other;
  };
  typedef T * pointer;
  typedef const T * const_pointer;
  typedef T & reference;
  typedef const T & const_reference;
  template<typename U, typename... Args>
  void construct(U * object, Args &&... args) {
    new (object) U(std::forward<Args>(args)...);
  }
  template<typename U, typename... Args>
  void construct(const U * object, Args &&... args) = delete;
  template<typename U>
  void destroy(U * object) {
    object->~U();
  }

private:
  union value_holder {
    value_holder() {}
    ~value_holder() {}
    T value;
  };

  std::vector<std::unique_ptr<value_holder[]>> memory;
  std::vector<T *> available;
};

#pragma once

#ifndef _FLAX_ARRAY_H_
#define _FLAX_ARRAY_H_

#include <cstdint>
#include <cstdlib>
#include <utility>

template <typename T>
class FlexArray {
  static_assert(std::is_trivial_v<T>, "FlexArray supports only trivial types");

 public:
  explicit FlexArray(const size_t size) noexcept : size_(size), buffer_(reinterpret_cast<T*>(std::malloc(size * sizeof(T)))) {
  }

  FlexArray(FlexArray&& other) noexcept : size_(other.size_), buffer_(other.buffer_) {
    other.buffer_ = nullptr;
    other.size_ = 0;
  }

  ~FlexArray() {
    if (buffer_ != nullptr) {
      std::free(buffer_);
    }
  }

  void Resize(const size_t size) noexcept {
    buffer_ = reinterpret_cast<T*>(std::realloc(buffer_, size * sizeof(T)));
    size_ = size;
  }

  size_t size() const noexcept {
    return size_;
  }

  T* data() const noexcept {
    return buffer_;
  }

 private:
  FlexArray(const FlexArray&) = delete;
  FlexArray& operator=(const FlexArray&) = delete;

  size_t size_ = 0;
  T* buffer_ = nullptr;
};

#endif
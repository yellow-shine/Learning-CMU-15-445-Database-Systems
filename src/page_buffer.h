#pragma once
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>

class PageBuffer {
 public:
  explicit PageBuffer(std::size_t size = 0)
      : bytes_(size ? std::make_unique<unsigned char[]>(size) : nullptr), size_(size) {}
  PageBuffer(const PageBuffer &) = delete;
  PageBuffer &operator=(const PageBuffer &) = delete;
  PageBuffer(PageBuffer &&other) noexcept
      : bytes_(std::move(other.bytes_)), size_(std::exchange(other.size_, 0)) {}
  PageBuffer &operator=(PageBuffer &&other) noexcept {
    if (this != &other) {
      bytes_ = std::move(other.bytes_);  // Releases our previous allocation once.
      size_ = std::exchange(other.size_, 0);
    }
    return *this;
  }
  ~PageBuffer() = default;
  std::size_t size() const noexcept { return size_; }
  const unsigned char *data() const noexcept { return bytes_.get(); }
  unsigned char &at(std::size_t index) {
    if (index >= size_) throw std::out_of_range("page byte offset");
    return bytes_[index];
  }
  const unsigned char &at(std::size_t index) const {
    if (index >= size_) throw std::out_of_range("page byte offset");
    return bytes_[index];
  }
 private:
  std::unique_ptr<unsigned char[]> bytes_;
  std::size_t size_;
};

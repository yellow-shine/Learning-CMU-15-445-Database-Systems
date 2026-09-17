#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

class LinearProbing {
  enum class State { empty, live, deleted };
  struct Slot { int key = 0; int value = 0; State state = State::empty; };
  std::vector<Slot> slots_;
  std::size_t size_ = 0;
  std::size_t home(int key) const { return static_cast<std::uint32_t>(key) % slots_.size(); }
public:
  explicit LinearProbing(std::size_t capacity) : slots_(capacity) {
    if (!capacity) throw std::invalid_argument("zero capacity");
  }
  std::size_t size() const { return size_; }
  std::optional<int> get(int key) const {
    for (std::size_t d = 0; d < slots_.size(); ++d) {
      const auto& s = slots_[(home(key) + d) % slots_.size()];
      if (s.state == State::empty) break;
      if (s.state == State::live && s.key == key) return s.value;
    }
    return std::nullopt;
  }
  bool put(int key, int value) {
    std::size_t free = slots_.size();
    for (std::size_t d = 0; d < slots_.size(); ++d) {
      const auto i = (home(key) + d) % slots_.size();
      auto& s = slots_[i];
      if (s.state == State::live && s.key == key) { s.value = value; return true; }
      if (s.state != State::live && free == slots_.size()) free = i;
      // A tombstone cannot stop the search: an existing key may follow it.
      if (s.state == State::empty) break;
    }
    if (free == slots_.size()) return false;
    slots_[free] = {key, value, State::live};
    ++size_;
    return true;
  }
  bool erase(int key) {
    for (std::size_t d = 0; d < slots_.size(); ++d) {
      auto& s = slots_[(home(key) + d) % slots_.size()];
      if (s.state == State::empty) break;
      if (s.state == State::live && s.key == key) {
        s.state = State::deleted; --size_; return true;
      }
    }
    return false;
  }
};

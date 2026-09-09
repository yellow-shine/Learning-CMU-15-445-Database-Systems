#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

class RobinHood {
public:
  struct Entry { int key; int value; std::size_t distance; };
private:
  std::vector<std::optional<Entry>> slots_;
  std::size_t size_ = 0;
  std::size_t home(int key) const { return static_cast<std::uint32_t>(key) % slots_.size(); }
  std::optional<std::size_t> locate(int key) const {
    for (std::size_t d = 0; d < slots_.size(); ++d) {
      const auto i = (home(key) + d) % slots_.size();
      if (!slots_[i] || slots_[i]->distance < d) break;
      if (slots_[i]->key == key) return i;
    }
    return std::nullopt;
  }
public:
  explicit RobinHood(std::size_t capacity) : slots_(capacity) {
    if (!capacity) throw std::invalid_argument("zero capacity");
  }
  std::size_t size() const { return size_; }
  const auto& slots() const { return slots_; }
  std::optional<int> get(int key) const {
    auto i = locate(key);
    return i ? std::optional<int>{slots_[*i]->value} : std::nullopt;
  }
  bool put(int key, int value) {
    if (auto i = locate(key)) { slots_[*i]->value = value; return true; }
    // Check before any swaps: a full-table failure must not lose a resident.
    if (size_ == slots_.size()) return false;
    Entry incoming{key, value, 0};
    auto i = home(key);
    for (;;) {
      if (!slots_[i]) { slots_[i] = incoming; ++size_; return true; }
      if (slots_[i]->distance < incoming.distance) std::swap(*slots_[i], incoming);
      i = (i + 1) % slots_.size();
      ++incoming.distance;
    }
  }
  bool erase(int key) {
    auto found = locate(key);
    if (!found) return false;
    auto hole = *found;
    slots_[hole].reset();
    for (std::size_t n = 0; n + 1 < slots_.size(); ++n) {
      const auto next = (hole + 1) % slots_.size();
      if (!slots_[next] || slots_[next]->distance == 0) break;
      slots_[hole] = slots_[next];
      --slots_[hole]->distance;
      slots_[next].reset();
      hole = next;
    }
    --size_;
    return true;
  }
  bool valid() const {
    std::size_t count = 0;
    for (std::size_t i = 0; i < slots_.size(); ++i) {
      if (!slots_[i]) continue;
      ++count;
      const auto& e = *slots_[i];
      if (e.distance != (i + slots_.size() - home(e.key)) % slots_.size()) return false;
      // Early-stop rule must not hide any resident (also catches duplicates).
      if (locate(e.key) != i) return false;
    }
    return count == size_;
  }
};

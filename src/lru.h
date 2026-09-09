#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

class LRU {
 public:
  explicit LRU(std::size_t capacity) : entries_(capacity) {}
  void access(std::size_t frame) {
    auto& e = entries_.at(frame);
    if (clock_ == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("logical clock");
    e.last = ++clock_;
  }
  void set_evictable(std::size_t frame, bool value) {
    auto& e = entries_.at(frame);
    if (!e.last) throw std::logic_error("unknown frame");
    e.evictable = value;
  }
  std::optional<std::size_t> evict() {
    std::optional<std::size_t> victim;
    // ponytail: O(F) timestamp scan; linked recency list if selection is a bottleneck.
    for (std::size_t i = 0; i < entries_.size(); ++i) {
      const auto& e = entries_[i];
      if (e.last && e.evictable && (!victim || *e.last < *entries_[*victim].last)) victim = i;
    }
    if (victim) entries_[*victim] = Entry{};
    return victim;
  }
 private:
  struct Entry { std::optional<std::uint64_t> last; bool evictable = false; };
  std::vector<Entry> entries_;
  std::uint64_t clock_ = 0;
};

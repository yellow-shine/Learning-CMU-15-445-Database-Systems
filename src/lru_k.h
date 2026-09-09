#pragma once
#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>
class LRUK {
 public:
  LRUK(std::size_t capacity, std::size_t k) : entries_(capacity), k_(k) {
    if (!k) throw std::invalid_argument("K must be positive");
  }
  void access(std::size_t id) {
    auto& e = entries_.at(id);
    if (clock_ == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("logical clock");
    e.history.push_back(clock_ + 1);
    ++clock_;
    if (e.history.size() > k_) e.history.pop_front();
  }
  void set_evictable(std::size_t id, bool value) {
    auto& e = entries_.at(id);
    if (e.history.empty()) throw std::logic_error("unknown frame");
    e.evictable = value;
  }
  std::optional<std::size_t> evict() {
    std::optional<std::size_t> victim;
    // ponytail: scan O(F); indexed candidate queues only if pool scale warrants them.
    for (std::size_t i = 0; i < entries_.size(); ++i) {
      const auto& e = entries_[i];
      if (e.history.empty() || !e.evictable) continue;
      if (!victim || before(e, entries_[*victim])) victim = i;
    }
    if (victim) { entries_[*victim].history.clear(); entries_[*victim].evictable = false; }
    return victim;
  }
 private:
  struct Entry { std::deque<std::uint64_t> history; bool evictable = false; };
  bool before(const Entry& a, const Entry& b) const {
    const bool a_cold = a.history.size() < k_, b_cold = b.history.size() < k_;
    if (a_cold != b_cold) return a_cold;
    // Cold: earliest first access. Hot: earliest Kth-most-recent access.
    // Equal timestamps keep the smaller frame ID (ascending scan).
    return a.history.front() < b.history.front();
  }
  std::vector<Entry> entries_;
  std::size_t k_;
  std::uint64_t clock_ = 0;
};

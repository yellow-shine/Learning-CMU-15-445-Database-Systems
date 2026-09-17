#pragma once
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <vector>
class Clock {
 public:
  explicit Clock(std::size_t capacity) : entries_(capacity) {}
  void access(std::size_t id) {
    auto& e = entries_.at(id); e.present = true; e.referenced = true;
  }
  void set_evictable(std::size_t id, bool value) {
    auto& e = entries_.at(id);
    if (!e.present) throw std::logic_error("unknown frame");
    e.evictable = value;
  }
  std::optional<std::size_t> evict() {
    // Two complete revolutions suffice: first clears bits, second finds a victim.
    for (int revolution = 0; revolution < 2; ++revolution) {
      for (std::size_t scanned = 0; scanned < entries_.size(); ++scanned) {
        auto id = hand_;
        hand_ = (hand_ + 1) % entries_.size();
        auto& e = entries_[id];
        if (!e.present || !e.evictable) continue; // Pinned reference bits are retained.
        if (e.referenced) { e.referenced = false; continue; }
        e = Entry{};
        return id;
      }
    }
    return std::nullopt;
  }
 private:
  struct Entry { bool present = false; bool referenced = false; bool evictable = false; };
  std::vector<Entry> entries_;
  std::size_t hand_ = 0;
};

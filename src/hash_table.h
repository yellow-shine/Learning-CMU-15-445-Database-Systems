#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

class CuckooHash {
  struct Entry { int key; int value; };
  using Tables = std::array<std::vector<std::optional<Entry>>, 2>;
  Tables tables_;
  std::uint64_t seed_ = 1;
  std::size_t size_ = 0;
  std::size_t max_capacity_;
  unsigned max_rebuilds_;
  static std::uint64_t mix(std::uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
  }
  static std::size_t position(int key, std::size_t table, std::size_t capacity,
                              std::uint64_t seed) {
    return mix(static_cast<std::uint32_t>(key) ^ mix(seed + table)) % capacity;
  }
public:
  struct Stats { std::size_t kicks = 0; std::size_t cycles = 0; unsigned rebuilds = 0; };
private:
  Stats last_;
  bool place(Tables& tables, Entry incoming, std::uint64_t seed) {
    std::size_t table = 0;
    std::vector<std::pair<int, std::size_t>> seen;
    const auto capacity = tables[0].size();
    // ponytail: conservative repeated-state detection; a stash can reduce rebuilds.
    for (std::size_t step = 0; step < 8 * capacity; ++step) {
      auto state = std::make_pair(incoming.key, table);
      if (std::find(seen.begin(), seen.end(), state) != seen.end()) {
        ++last_.cycles; return false;
      }
      seen.push_back(state);
      auto& slot = tables[table][position(incoming.key, table, capacity, seed)];
      if (!slot) { slot = incoming; return true; }
      std::swap(*slot, incoming);
      ++last_.kicks;
      table = 1 - table;
    }
    return false; // Kick budget is independent of cycle detection.
  }
public:
  explicit CuckooHash(std::size_t capacity = 4, std::size_t max_capacity = 65536,
                      unsigned max_rebuilds = 6)
      : max_capacity_(max_capacity), max_rebuilds_(max_rebuilds) {
    if (!capacity || capacity > max_capacity || max_capacity > 65536 || max_rebuilds > 16)
      throw std::invalid_argument("capacity must be 1..65536; rebuilds <= 16");
    for (auto& table : tables_) table.resize(capacity);
  }
  std::size_t size() const { return size_; }
  std::size_t capacity() const { return tables_[0].size(); }
  Stats last_stats() const { return last_; }
  std::optional<int> get(int key) const {
    for (std::size_t t = 0; t < 2; ++t) {
      const auto& e = tables_[t][position(key, t, capacity(), seed_)];
      if (e && e->key == key) return e->value;
    }
    return std::nullopt;
  }
  bool erase(int key) {
    for (std::size_t t = 0; t < 2; ++t) {
      auto& e = tables_[t][position(key, t, capacity(), seed_)];
      if (e && e->key == key) { e.reset(); --size_; return true; }
    }
    return false;
  }
  bool put(int key, int value) {
    last_ = {};
    for (std::size_t t = 0; t < 2; ++t) {
      auto& e = tables_[t][position(key, t, capacity(), seed_)];
      if (e && e->key == key) { e->value = value; return true; }
    }
    // Work on a snapshot: failed eviction/rebuild never damages committed keys.
    auto candidate = tables_;
    if (place(candidate, {key, value}, seed_)) {
      tables_.swap(candidate); ++size_; return true;
    }
    std::vector<Entry> entries;
    for (const auto& table : tables_) for (const auto& e : table) if (e) entries.push_back(*e);
    entries.push_back({key, value});
    auto cap = capacity();
    for (unsigned attempt = 1; attempt <= max_rebuilds_; ++attempt) {
      ++last_.rebuilds;
      cap = std::min(cap * 2, max_capacity_);
      Tables rebuilt;
      for (auto& table : rebuilt) table.resize(cap);
      bool ok = true;
      for (const auto& e : entries) if (!place(rebuilt, e, seed_ + attempt)) { ok = false; break; }
      if (ok) { tables_.swap(rebuilt); seed_ += attempt; ++size_; return true; }
    }
    return false;
  }
  bool valid() const {
    std::size_t count = 0;
    for (std::size_t t = 0; t < 2; ++t) {
      for (std::size_t i = 0; i < capacity(); ++i) if (const auto& e = tables_[t][i]) {
        ++count;
        if (position(e->key, t, capacity(), seed_) != i) return false;
        const auto& other = tables_[1-t][position(e->key, 1-t, capacity(), seed_)];
        if (other && other->key == e->key) return false;
      }
    }
    return count == size_;
  }
};

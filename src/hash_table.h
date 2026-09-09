#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <tuple>
#include <vector>

struct RID {
  std::uint32_t page;
  std::uint32_t slot;
  bool operator==(const RID& other) const { return page == other.page && slot == other.slot; }
  bool operator<(const RID& other) const { return std::tie(page, slot) < std::tie(other.page, other.slot); }
};

class HashIndex {
  struct Entry { int key; std::vector<RID> rids; };
  using Bucket = std::vector<Entry>;
  std::vector<Bucket> buckets_;
  std::size_t pairs_ = 0;
  std::size_t keys_ = 0;
  std::size_t bucket_for(int key) const { return static_cast<std::uint32_t>(key) % buckets_.size(); }
public:
  explicit HashIndex(std::size_t bucket_count = 17) : buckets_(bucket_count) {
    if (!bucket_count) throw std::invalid_argument("zero buckets");
  }
  std::size_t size() const { return pairs_; }
  std::size_t key_count() const { return keys_; }
  bool insert(int key, RID rid) {
    auto& bucket = buckets_[bucket_for(key)];
    for (auto& e : bucket) if (e.key == key) {
      if (std::find(e.rids.begin(), e.rids.end(), rid) != e.rids.end()) return false;
      e.rids.push_back(rid); ++pairs_; return true;
    }
    bucket.push_back(Entry{key, {rid}});
    ++pairs_; ++keys_;
    return true;
  }
  std::vector<RID> lookup(int key) const {
    for (const auto& e : buckets_[bucket_for(key)]) if (e.key == key) return e.rids;
    return {};
  }
  bool erase(int key, RID rid) {
    auto& bucket = buckets_[bucket_for(key)];
    for (auto e = bucket.begin(); e != bucket.end(); ++e) if (e->key == key) {
      auto r = std::find(e->rids.begin(), e->rids.end(), rid);
      if (r == e->rids.end()) return false;
      e->rids.erase(r); --pairs_;
      if (e->rids.empty()) { bucket.erase(e); --keys_; }
      return true;
    }
    return false;
  }
  bool valid() const {
    std::size_t pairs = 0, keys = 0;
    for (std::size_t i = 0; i < buckets_.size(); ++i) {
      const auto& bucket = buckets_[i];
      for (std::size_t j = 0; j < bucket.size(); ++j) {
        const auto& e = bucket[j];
        if (bucket_for(e.key) != i || e.rids.empty()) return false;
        for (std::size_t k = 0; k < j; ++k) if (bucket[k].key == e.key) return false;
        for (std::size_t r = 0; r < e.rids.size(); ++r)
          for (std::size_t s = 0; s < r; ++s) if (e.rids[r] == e.rids[s]) return false;
        ++keys; pairs += e.rids.size();
      }
    }
    return pairs == pairs_ && keys == keys_;
  }
};

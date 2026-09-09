#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

class ExtendibleHash {
  struct Bucket { unsigned depth; std::vector<std::pair<int, int>> entries; };
  std::vector<std::shared_ptr<Bucket>> directory_;
  unsigned global_depth_ = 0;
  unsigned max_depth_;
  std::size_t bucket_capacity_;
  std::size_t size_ = 0;
  static std::size_t mask(unsigned depth) { return (std::size_t{1} << depth) - 1; }
  std::size_t index(int key) const { return static_cast<std::uint32_t>(key) & mask(global_depth_); }
  void split(const std::shared_ptr<Bucket>& old) {
    const auto bit = std::size_t{1} << old->depth;
    auto left = std::make_shared<Bucket>(Bucket{old->depth + 1, {}});
    auto right = std::make_shared<Bucket>(Bucket{old->depth + 1, {}});
    for (const auto& e : old->entries)
      ((static_cast<std::uint32_t>(e.first) & bit) ? right : left)->entries.push_back(e);
    // Publish only after all allocations: old buckets remain valid on exception.
    auto next = directory_;
    if (old->depth == global_depth_) {
      const auto n = next.size();
      next.resize(2 * n);
      for (std::size_t i = 0; i < n; ++i) next[i + n] = next[i];
    }
    for (std::size_t i = 0; i < next.size(); ++i)
      if (next[i] == old) next[i] = (i & bit) ? right : left;
    const bool doubled = old->depth == global_depth_;
    directory_.swap(next);
    if (doubled) ++global_depth_;
  }
public:
  explicit ExtendibleHash(std::size_t bucket_capacity = 2, unsigned max_depth = 16)
      : max_depth_(max_depth), bucket_capacity_(bucket_capacity) {
    if (!bucket_capacity || max_depth > 16) throw std::invalid_argument("capacity > 0, depth <= 16 required");
    directory_.push_back(std::make_shared<Bucket>(Bucket{0, {}}));
  }
  // Directory aliases are internal; copying shared buckets would couple two tables.
  ExtendibleHash(const ExtendibleHash&) = delete;
  ExtendibleHash& operator=(const ExtendibleHash&) = delete;
  std::size_t size() const { return size_; }
  unsigned global_depth() const { return global_depth_; }
  std::size_t directory_size() const { return directory_.size(); }
  unsigned local_depth(std::size_t i) const { return directory_.at(i)->depth; }
  bool same_bucket(std::size_t a, std::size_t b) const { return directory_.at(a) == directory_.at(b); }
  std::optional<int> get(int key) const {
    for (const auto& e : directory_[index(key)]->entries) if (e.first == key) return e.second;
    return std::nullopt;
  }
  bool put(int key, int value) {
    for (;;) {
      auto bucket = directory_[index(key)];
      for (auto& e : bucket->entries) if (e.first == key) { e.second = value; return true; }
      if (bucket->entries.size() < bucket_capacity_) {
        bucket->entries.emplace_back(key, value); ++size_; return true;
      }
      if (bucket->depth == max_depth_) return false;
      split(bucket);
    }
  }
  bool erase(int key) {
    auto& entries = directory_[index(key)]->entries;
    for (auto it = entries.begin(); it != entries.end(); ++it)
      if (it->first == key) { entries.erase(it); --size_; return true; }
    return false;
  }
  bool valid() const {
    if (directory_.size() != (std::size_t{1} << global_depth_) || global_depth_ > max_depth_) return false;
    std::vector<std::size_t> references(directory_.size(), 0);
    for (std::size_t i = 0; i < directory_.size(); ++i) {
      const auto& b = directory_[i];
      if (!b || b->depth > global_depth_ || b->entries.size() > bucket_capacity_) return false;
      auto prefix = i & mask(b->depth);
      if (directory_[prefix] != b) return false;
      ++references[prefix];
    }
    std::size_t count = 0;
    for (std::size_t i = 0; i < references.size(); ++i) if (references[i]) {
      const auto& b = directory_[i];
      if (references[i] != (std::size_t{1} << (global_depth_ - b->depth))) return false;
      count += b->entries.size();
      for (std::size_t j = 0; j < b->entries.size(); ++j) {
        if (directory_[index(b->entries[j].first)] != b) return false;
        for (std::size_t k = 0; k < j; ++k)
          if (b->entries[k].first == b->entries[j].first) return false;
      }
    }
    return count == size_;
  }
};

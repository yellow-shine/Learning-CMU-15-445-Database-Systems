#pragma once
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

// Values are copied out while the read lock is held; no borrowed map entries.
class PageDirectory {
 public:
  void set(int page, int offset) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    offsets_[page] = offset;
  }
  std::optional<int> get(int page) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = offsets_.find(page);
    if (it == offsets_.end()) return std::nullopt;
    return it->second;
  }
 private:
  mutable std::shared_mutex mutex_;
  std::unordered_map<int,int> offsets_;
};

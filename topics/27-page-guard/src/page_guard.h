#pragma once
#include "buffer_pool.h"
#include <type_traits>
#include <utility>

template<bool Write>
class PageGuard {
  using Lock = std::conditional_t<Write, std::unique_lock<std::shared_mutex>,
                                  std::shared_lock<std::shared_mutex>>;
 public:
  PageGuard() = default;
  PageGuard(BufferPool& pool, std::size_t id) : pool_(&pool), frame_(&pool.fetch(id)) {
    try { lock_ = Lock(frame_->latch); }
    catch (...) { pool.unpin(id); throw; }
  }
  ~PageGuard() { drop(); }
  PageGuard(const PageGuard&) = delete;
  PageGuard& operator=(const PageGuard&) = delete;
  PageGuard(PageGuard&& other) noexcept
      : pool_(std::exchange(other.pool_, nullptr)), frame_(std::exchange(other.frame_, nullptr)),
        lock_(std::move(other.lock_)), dirty_(std::exchange(other.dirty_, false)) {}
  PageGuard& operator=(PageGuard&& other) noexcept {
    if (this != &other) {
      drop();
      pool_ = std::exchange(other.pool_, nullptr);
      frame_ = std::exchange(other.frame_, nullptr);
      lock_ = std::move(other.lock_);
      dirty_ = std::exchange(other.dirty_, false);
    }
    return *this;
  }
  const Page& data() const { check(); return frame_->data; }
  Page& mutable_data() {
    static_assert(Write, "read guards cannot modify pages");
    check(); dirty_ = true; return frame_->data;
  }
  void drop() noexcept {
    if (!pool_) return;
    auto id = *frame_->page_id;
    lock_.unlock(); // Release latch BEFORE taking pool metadata lock in unpin.
    pool_->unpin(id, dirty_);
    pool_ = nullptr; frame_ = nullptr; dirty_ = false;
  }
 private:
  void check() const { if (!frame_) throw std::logic_error("empty page guard"); }
  BufferPool* pool_ = nullptr;
  Frame* frame_ = nullptr;
  Lock lock_;
  bool dirty_ = false;
};
using ReadPageGuard = PageGuard<false>;
using WritePageGuard = PageGuard<true>;

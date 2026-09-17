#pragma once
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <stdexcept>

class WorkQueue {
 public:
  explicit WorkQueue(std::size_t capacity) : capacity_(capacity) {
    if (!capacity) throw std::invalid_argument("zero capacity");
  }
  bool push(int page) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!closed_ && queue_.size() == capacity_) {
      ++waiting_push_; observed_.notify_all();
      writable_.wait(lock, [&] { return closed_ || queue_.size() < capacity_; });
      --waiting_push_;
    }
    if (closed_) return false;
    queue_.push_back(page);
    readable_.notify_one();
    return true;
  }
  std::optional<int> pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!closed_ && queue_.empty()) {
      ++waiting_pop_; observed_.notify_all();
      readable_.wait(lock, [&] { return closed_ || !queue_.empty(); });
      --waiting_pop_;
    }
    if (queue_.empty()) return std::nullopt;
    int page = queue_.front(); queue_.pop_front();
    writable_.notify_one();
    return page;
  }
  void close() {
    std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;
    readable_.notify_all(); writable_.notify_all();
  }
  // Teaching observation: confirms waiters under the same mutex, not a sleep.
  bool wait_for_waiters(std::size_t producers, std::size_t consumers) {
    std::unique_lock<std::mutex> lock(mutex_);
    return observed_.wait_for(lock, std::chrono::seconds(2), [&] {
      return waiting_push_ >= producers && waiting_pop_ >= consumers;
    });
  }
 private:
  const std::size_t capacity_;
  std::mutex mutex_;
  std::condition_variable readable_, writable_, observed_;
  std::deque<int> queue_;
  bool closed_ = false;
  std::size_t waiting_push_ = 0, waiting_pop_ = 0;
};

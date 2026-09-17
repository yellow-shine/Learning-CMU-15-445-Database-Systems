#pragma once
#include "disk.h"
#include <condition_variable>
#include <deque>
#include <future>
#include <thread>

// Disk must outlive scheduler. Requests own their bytes, never caller buffers.
class DiskScheduler {
 public:
  explicit DiskScheduler(Disk& disk) : disk_(disk), worker_([this] { run(); }) {}
  ~DiskScheduler() { close(); }
  DiskScheduler(const DiskScheduler&) = delete;
  DiskScheduler& operator=(const DiskScheduler&) = delete;
  std::future<Page> read(std::size_t id) { return submit(false, id, Page{}); }
  // Successful writes return the written page as an acknowledgement.
  std::future<Page> write(std::size_t id, Page page) { return submit(true, id, std::move(page)); }
  void close() {
    std::lock_guard<std::mutex> closer(close_mutex_); // Concurrent close calls join exactly once.
    { std::lock_guard<std::mutex> lock(mutex_); stopping_ = true; }
    ready_.notify_one();
    if (worker_.joinable()) worker_.join(); // Do not hold the queue lock while draining.
  }
 private:
  struct Request {
    bool writing;
    std::size_t id;
    Page page;
    std::promise<Page> done;
  };
  std::future<Page> submit(bool writing, std::size_t id, Page page) {
    Request request{writing, id, std::move(page), {}};
    auto future = request.done.get_future();
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (stopping_) throw std::logic_error("scheduler closed");
      queue_.push_back(std::move(request));
    }
    ready_.notify_one();
    return future;
  }
  void run() {
    for (;;) {
      Request request;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        ready_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
        if (queue_.empty()) return;
        request = std::move(queue_.front()); queue_.pop_front();
      }
      try {
        if (request.writing) disk_.write(request.id, request.page);
        else request.page = disk_.read(request.id);
        request.done.set_value(std::move(request.page));
      } catch (...) { request.done.set_exception(std::current_exception()); }
    }
  }
  Disk& disk_;
  std::mutex mutex_, close_mutex_;
  std::condition_variable ready_;
  std::deque<Request> queue_;
  bool stopping_ = false;
  std::thread worker_; // Last: all state exists before worker starts.
};

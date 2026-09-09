#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <stdexcept>

using Page = std::array<char, 256>;
// Fixed-size existing file; no implicit page allocation or sparse reads.
class Disk {
 public:
  explicit Disk(std::filesystem::path path) : path_(std::move(path)) {
    const auto bytes = std::filesystem::file_size(path_);
    if (bytes % Page{}.size() || bytes > static_cast<std::uintmax_t>(std::numeric_limits<std::streamoff>::max()))
      throw std::runtime_error("invalid page file length");
    count_ = bytes / Page{}.size();
  }
  Page read(std::size_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    check(id);
    std::ifstream in(path_, std::ios::binary);
    Page page{};
    in.seekg(static_cast<std::streamoff>(id * page.size()));
    in.read(page.data(), page.size());
    if (!in) throw std::runtime_error("page read failed");
    return page;
  }
  void write(std::size_t id, const Page& page) {
    std::lock_guard<std::mutex> lock(mutex_);
    check(id);
    std::fstream out(path_, std::ios::binary | std::ios::in | std::ios::out);
    out.seekp(static_cast<std::streamoff>(id * page.size()));
    out.write(page.data(), page.size());
    out.flush();
    if (!out) throw std::runtime_error("page write failed");
  }
 private:
  void check(std::size_t id) const { if (id >= count_) throw std::out_of_range("page id"); }
  std::filesystem::path path_;
  std::size_t count_{};
  std::mutex mutex_;
};

#pragma once
#include "page.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace storage {
// One owner; no concurrent writers. The on-disk format is just consecutive pages.
class PageFile {
  std::fstream file_;
  std::uint64_t pages_ = 0;
  static std::streamoff offset(std::uint64_t id) {
    if (id > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max()) / page_size)
      throw std::out_of_range("page offset overflow");
    return static_cast<std::streamoff>(id * page_size);
  }
 public:
  explicit PageFile(const std::filesystem::path& path, bool create = false) {
    if (create) {
      // Never truncate an existing file, even on accidental repeated creation.
      if (std::filesystem::exists(path)) throw std::runtime_error("file already exists");
      std::ofstream fresh(path, std::ios::binary);
      if (!fresh) throw std::runtime_error("create failed");
    }
    file_.open(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!file_) throw std::runtime_error("open failed");
    file_.seekg(0, std::ios::end);
    const auto bytes = file_.tellg();
    if (bytes < 0 || bytes % static_cast<std::streamoff>(page_size) != 0)
      throw std::runtime_error("partial page file");
    pages_ = static_cast<std::uint64_t>(bytes) / page_size;
  }
  std::uint64_t size() const { return pages_; }
  Page read(std::uint64_t id) {
    if (id >= pages_) throw std::out_of_range("invalid page id");
    Page page{};
    file_.clear(); file_.seekg(offset(id));
    file_.read(reinterpret_cast<char*>(page.data()), page.size());
    if (!file_ || file_.gcount() != static_cast<std::streamsize>(page.size()))
      throw std::runtime_error("short page read");
    return page;
  }
  void write(std::uint64_t id, const Page& page) {
    if (id >= pages_) throw std::out_of_range("invalid page id");
    write_at(id, page);
  }
  std::uint64_t append(const Page& page = {}) {
    offset(pages_ + 1); // validate the end offset before changing the file
    write_at(pages_, page);
    return pages_++;
  }
  void flush() {
    file_.flush();
    if (!file_) throw std::runtime_error("flush failed");
  }
 private:
  void write_at(std::uint64_t id, const Page& page) {
    file_.clear(); file_.seekp(offset(id));
    file_.write(reinterpret_cast<const char*>(page.data()), page.size());
    if (!file_) throw std::runtime_error("page write failed");
    flush();
  }
};
} // namespace storage

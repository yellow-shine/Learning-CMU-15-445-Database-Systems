#pragma once
#include <cstdio>
#include <stdexcept>
#include <string>

// One object owns one FILE. Construction either owns it or throws.
class FileGuard {
 public:
  explicit FileGuard(const std::string &path, const char *mode = "wb")
      : file_(std::fopen(path.c_str(), mode)) {
    if (!file_) throw std::runtime_error("cannot open: " + path);
  }
  FileGuard(const FileGuard &) = delete;
  FileGuard &operator=(const FileGuard &) = delete;
  ~FileGuard() noexcept { if (file_) std::fclose(file_); }

  void write(const std::string &bytes) {
    if (!file_) throw std::logic_error("file is closed");
    if (std::fwrite(bytes.data(), 1, bytes.size(), file_) != bytes.size())
      throw std::runtime_error("write failed");
  }
  // Explicit close reports buffered write errors; the destructor cannot.
  void close() {
    if (!file_) return;
    auto *owned = file_;
    file_ = nullptr;
    if (std::fclose(owned) != 0) throw std::runtime_error("close failed");
  }
  std::FILE *borrow() const noexcept { return file_; }
 private:
  std::FILE *file_;
};

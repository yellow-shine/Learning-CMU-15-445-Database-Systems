#pragma once
#include "disk.h"
#include <chrono>
#include <string>
// Demo/test-owned directory: atomic creation avoids overwriting existing data.
class TempFile {
 public:
  explicit TempFile(std::size_t pages) {
    auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
    for (unsigned i = 0; i < 1000; ++i) {
      directory_ = std::filesystem::temp_directory_path() /
          ("cmu445-pages-" + std::to_string(seed) + "-" + std::to_string(i));
      if (std::filesystem::create_directory(directory_)) {
        try {
          std::ofstream out(path(), std::ios::binary);
          Page zero{};
          for (std::size_t p = 0; p < pages; ++p) out.write(zero.data(), zero.size());
          out.close();
          if (!out) throw std::runtime_error("initialize file failed");
        } catch (...) { std::filesystem::remove_all(directory_); throw; }
        return;
      }
    }
    throw std::runtime_error("cannot create temporary directory");
  }
  ~TempFile() { std::error_code ec; std::filesystem::remove_all(directory_, ec); }
  TempFile(const TempFile&) = delete;
  TempFile& operator=(const TempFile&) = delete;
  std::filesystem::path path() const { return directory_ / "pages.bin"; }
 private:
  std::filesystem::path directory_;
};

#pragma once
#include <cstdlib>
#include <unistd.h>
#include <filesystem>
#include <stdexcept>
struct TempDirectory {
  std::filesystem::path path;
  TempDirectory() {
    auto pattern = (std::filesystem::temp_directory_path() / "raii-XXXXXX").string();
    char *created = ::mkdtemp(pattern.data());
    if (!created) throw std::runtime_error("mkdtemp failed");
    path = created;
  }
  TempDirectory(const TempDirectory &) = delete;
  TempDirectory &operator=(const TempDirectory &) = delete;
  ~TempDirectory() { std::error_code ec; std::filesystem::remove_all(path, ec); }
};

#pragma once
#include <filesystem>
#include <chrono>
#include <stdexcept>
// Private test/demo directory, deleted only by its owner.
struct TempDir {
  std::filesystem::path path;
  TempDir() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    for (int i=0; i<1000; ++i) {
      auto p = std::filesystem::temp_directory_path() / ("storage-tutorial-" + std::to_string(stamp) + "-" + std::to_string(i));
      if (std::filesystem::create_directory(p)) { path=p; return; }
    }
    throw std::runtime_error("cannot create private directory");
  }
  TempDir(const TempDir&) = delete;
  TempDir& operator=(const TempDir&) = delete;
  ~TempDir() { std::error_code ec; std::filesystem::remove_all(path, ec); }
};

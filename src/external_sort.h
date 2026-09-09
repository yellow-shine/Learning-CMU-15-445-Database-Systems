#pragma once
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace tutorial {
namespace fs = std::filesystem;
using Value = std::int64_t;

// Own only a newly created directory; never adopt a caller's existing data.
class Workspace {
 public:
  explicit Workspace(const fs::path& parent = fs::temp_directory_path()) {
    std::random_device random;
    for (int attempt = 0; attempt < 100; ++attempt) {
      auto candidate = parent / ("external-sort-" + std::to_string(random()) + "-" + std::to_string(random()));
      if (fs::create_directory(candidate)) { path_ = candidate; return; }
    }
    throw std::runtime_error("cannot allocate workspace");
  }
  Workspace(const Workspace&) = delete;
  Workspace& operator=(const Workspace&) = delete;
  ~Workspace() { std::error_code ignored; fs::remove_all(path_, ignored); }
  const fs::path& path() const { return path_; }
 private:
  fs::path path_;
};

inline std::ifstream open_input(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("cannot open input: " + path.string());
  return in;
}
inline std::ofstream open_output(const fs::path& path) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) throw std::runtime_error("cannot open run: " + path.string());
  return out;
}
inline bool read_value(std::istream& in, Value& value) {
  in.read(reinterpret_cast<char*>(&value), sizeof(value));
  if (in.gcount() == sizeof(value) && !in.bad()) return true;
  if (in.gcount() == 0 && in.eof() && !in.bad()) return false;
  throw std::runtime_error("short or failed record read");
}
inline void write_value(std::ostream& out, Value value) {
  out.write(reinterpret_cast<const char*>(&value), sizeof(value));
  if (!out) throw std::runtime_error("record write failed");
}
inline void close_output(std::ofstream& out) {
  out.close();
  if (!out) throw std::runtime_error("run close failed");
}
struct SortResult {
  fs::path file;
  std::size_t initial_runs = 0, merge_passes = 0, peak_records = 0, peak_open_files = 0;
};

// A fresh workspace is required. Budget is the number of int64 records in
// generation; merge holds <= fan_in heads, and opens <= fan_in + 1 files.
inline SortResult external_sort(const fs::path& input, const Workspace& workspace,
                                std::size_t budget, std::size_t fan_in) {
  if (budget < 2 || fan_in < 2 || fan_in > budget)
    throw std::invalid_argument("require 2 <= fan_in <= record budget");
  if (!fs::is_empty(workspace.path())) throw std::invalid_argument("workspace must be empty");
  auto run = [&](std::size_t pass, std::size_t index) {
    return workspace.path() / (std::to_string(pass) + "-" + std::to_string(index) + ".run");
  };
  SortResult result;
  {
    auto in = open_input(input);
    std::vector<Value> chunk;
    chunk.reserve(budget);
    bool end = false;
    while (!end) {
      chunk.clear();
      Value value{};
      while (chunk.size() < budget) {
        if (!read_value(in, value)) { end = true; break; }
        chunk.push_back(value);
      }
      if (chunk.empty() && result.initial_runs != 0) break;
      std::sort(chunk.begin(), chunk.end());
      auto out = open_output(run(0, result.initial_runs++));
      for (auto item : chunk) write_value(out, item);
      close_output(out);
      result.peak_records = std::max(result.peak_records, chunk.size());
      result.peak_open_files = 2;
    }
  } // Release generation memory and input descriptor before merging.
  std::size_t count = result.initial_runs, pass = 0;
  while (count > 1) {
    std::size_t next_count = 0;
    for (std::size_t first = 0; first < count;) {
      const auto width = std::min(fan_in, count - first);
      {
        std::vector<std::ifstream> inputs;
        inputs.reserve(width);
        using Head = std::pair<Value, std::size_t>;
        std::vector<Head> storage;
        storage.reserve(width);
        std::priority_queue<Head, std::vector<Head>, std::greater<Head>> heap(std::greater<Head>{}, std::move(storage));
        for (std::size_t i = 0; i < width; ++i) {
          inputs.push_back(open_input(run(pass, first + i)));
          Value value{};
          if (read_value(inputs.back(), value)) heap.emplace(value, i);
        }
        auto out = open_output(run(pass + 1, next_count++));
        result.peak_open_files = std::max(result.peak_open_files, width + 1);
        result.peak_records = std::max(result.peak_records, heap.size());
        while (!heap.empty()) {
          const auto head = heap.top();
          heap.pop();
          write_value(out, head.first);
          Value value{};
          if (read_value(inputs[head.second], value)) heap.emplace(value, head.second);
        }
        close_output(out);
      }
      for (std::size_t i = 0; i < width; ++i) fs::remove(run(pass, first + i));
      first += width;
    }
    count = next_count;
    ++pass;
  }
  result.merge_passes = pass;
  result.file = run(pass, 0);
  return result;
}
} // namespace tutorial

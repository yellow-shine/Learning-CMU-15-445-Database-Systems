#pragma once
#include <algorithm>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
struct Row { int id; std::string region; bool deleted; };

// Own the input copy: callers retain their rows and iterators.
inline std::vector<int> live_ids(std::vector<Row> rows) {
  rows.erase(std::remove_if(rows.begin(), rows.end(),
                           [](const Row &r) { return r.deleted; }), rows.end());
  std::vector<int> ids;
  ids.reserve(rows.size());
  for (const auto &row : rows) ids.push_back(row.id);
  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  return ids;
}
inline std::vector<std::pair<std::string, std::size_t>> region_counts(const std::vector<Row> &rows) {
  std::unordered_map<std::string, std::size_t> counts;
  for (const auto &row : rows) if (!row.deleted) ++counts[row.region];
  std::vector<std::pair<std::string, std::size_t>> result(counts.begin(), counts.end());
  std::sort(result.begin(), result.end()); // Never expose hash iteration order.
  return result;
}

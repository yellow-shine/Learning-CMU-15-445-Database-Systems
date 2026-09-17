#include "aggregation.h"
#include <iostream>
#include <string>
template<class T> std::string text(std::optional<T> v) {
  return v ? std::to_string(*v) : "NULL";
}
int main() {
  try {
    const std::vector<tutorial::Row> rows{{2, 10}, {1, -4}, {2, std::nullopt},
      {1, 6}, {std::nullopt, 3}, {2, 20}, {3, std::nullopt}};
    std::cout << "key count(*) count(v) sum avg min max\n";
    auto groups = tutorial::hash_aggregate(rows);
    // Display order is not the hash aggregation contract.
    std::sort(groups.begin(), groups.end(), [](const auto& a, const auto& b) { return a.key < b.key; });
    for (const auto& group : groups) {
      const auto& s = group.state;
      std::cout << text(group.key) << ' ' << s.count_star() << ' ' << s.count_column() << ' '
                << text(s.sum()) << ' ' << text(s.avg()) << ' ' << text(s.min()) << ' ' << text(s.max()) << '\n';
    }
    auto empty = tutorial::global_aggregate({});
    std::cout << "empty global: " << empty.count_star() << ' ' << text(empty.sum()) << '\n';
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

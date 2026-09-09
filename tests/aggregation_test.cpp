#include "aggregation.h"
#include <cmath>
#include <iostream>
#include <random>
#include <unordered_map>
using namespace tutorial;
void check(bool ok) { if (!ok) throw std::runtime_error("aggregation check failed"); }
bool equal(const State& a, const State& b) {
  return a.count_star() == b.count_star() && a.count_column() == b.count_column() &&
    a.sum() == b.sum() && a.avg() == b.avg() && a.min() == b.min() && a.max() == b.max();
}
// Independent O(NG) oracle: discover keys without sorting, then rescan each group.
void verify(const std::vector<Row>& rows) {
  auto actual = sort_aggregate(rows);
  // Cross-check against hash grouping as well as the independent scalar oracle.
  std::unordered_map<Key, State> hashed;
  for (const auto& row : rows) hashed[row.key].add(row.value);
  check(actual.size() == hashed.size());
  for (const auto& group : actual) check(equal(group.state, hashed.at(group.key)));
  std::vector<Key> keys;
  for (const auto& row : rows)
    if (std::find(keys.begin(), keys.end(), row.key) == keys.end()) keys.push_back(row.key);
  check(actual.size() == keys.size());
  for (const auto& key : keys) {
    std::uint64_t count = 0, nonnull = 0;
    Value sum = 0;
    std::optional<Value> min, max;
    for (const auto& row : rows) if (row.key == key) {
      ++count;
      if (row.value) {
        ++nonnull; sum += *row.value;
        if (!min || *row.value < *min) min = row.value;
        if (!max || *row.value > *max) max = row.value;
      }
    }
    auto found = std::find_if(actual.begin(), actual.end(), [&](const Group& g) { return g.key == key; });
    check(found != actual.end());
    const auto& s = found->state;
    check(s.count_star() == count && s.count_column() == nonnull && s.min() == min && s.max() == max);
    check(s.sum() == (nonnull ? std::optional<Value>(sum) : std::nullopt));
    check(s.avg().has_value() == (nonnull != 0));
    if (nonnull) check(std::fabs(*s.avg() - static_cast<long double>(sum) / nonnull) < 1e-12L);
  }
  for (std::size_t i = 1; i < actual.size(); ++i) check(actual[i-1].key < actual[i].key);
}
template<class F> void overflow(F fn) {
  bool caught = false;
  try { fn(); } catch (const std::overflow_error&) { caught = true; }
  check(caught);
}
int main() {
  try {
    verify({}); verify({{1, std::nullopt}});
    verify({{2, 10}, {1, -4}, {2, std::nullopt}, {1, 6}, {std::nullopt, 3}, {2, 20}, {3, std::nullopt}});
    verify({{std::nullopt, std::nullopt}, {std::nullopt, 0}, {1, -8}, {1, -8}});
    auto empty = global_aggregate({});
    check(empty.count_star() == 0 && empty.count_column() == 0 && !empty.sum() && !empty.avg() && !empty.min() && !empty.max());
    State nulls; nulls.add(std::nullopt); nulls.add(std::nullopt);
    check(nulls.count_star() == 2 && nulls.count_column() == 0 && !nulls.sum() && !nulls.avg());
    State a, b; a.add(10); b.add(20); b.add(30); b.add(40); b.add(std::nullopt);
    a.merge(b); check(a.count_star() == 5 && a.count_column() == 4 && *a.avg() == 25);
    // Unequal partitions: (10 + 30)/2 = 20 would be wrong.
    State high; high.add(std::numeric_limits<Value>::max()); auto saved = high;
    overflow([&] { high.add(1); }); check(equal(high, saved));
    State low; low.add(std::numeric_limits<Value>::min()); saved = low;
    overflow([&] { low.add(-1); }); check(equal(low, saved));
    State many; many.add(0);
    for (int i = 0; i < 63; ++i) many.merge(many);
    saved = many; overflow([&] { many.merge(many); }); check(equal(many, saved));
    std::mt19937 random(445);
    std::vector<Row> rows;
    for (int i = 0; i < 2000; ++i) {
      Key key = i % 13 ? Key(static_cast<int>(random() % 31) - 15) : std::nullopt;
      std::optional<Value> value = i % 7 ? std::optional<Value>(static_cast<int>(random() % 2001) - 1000) : std::nullopt;
      rows.push_back({key, value});
    }
    verify(rows);
    State left, right;
    for (std::size_t i = 0; i < rows.size(); ++i) (i < 173 ? left : right).add(rows[i].value);
    left.merge(right); check(equal(left, global_aggregate(rows)));
    std::reverse(rows.begin(), rows.end()); verify(rows);
    std::cout << "aggregation: all checks passed\n";
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

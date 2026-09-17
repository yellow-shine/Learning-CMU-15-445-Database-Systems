#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

namespace tutorial {
using Value = std::int64_t;
using Key = std::optional<int>;
struct Row { Key key; std::optional<Value> value; };

class State {
 public:
  std::uint64_t count_star() const { return rows_; }
  std::uint64_t count_column() const { return count_; }
  std::optional<Value> sum() const { return count_ ? std::optional<Value>(sum_) : std::nullopt; }
  std::optional<Value> min() const { return min_; }
  std::optional<Value> max() const { return max_; }
  std::optional<long double> avg() const {
    return count_ ? std::optional<long double>(static_cast<long double>(sum_) / count_) : std::nullopt;
  }
  void add(std::optional<Value> value) {
    State one;
    one.rows_ = 1;
    if (value) { one.count_ = 1; one.sum_ = *value; one.min_ = one.max_ = value; }
    merge(one);
  }
  // Merge sufficient statistics, never average local averages. Commit only
  // after all overflow checks, so a rejected update leaves this state intact.
  void merge(const State& other) {
    const auto limit = std::numeric_limits<std::uint64_t>::max();
    if (other.rows_ > limit - rows_ || other.count_ > limit - count_)
      throw std::overflow_error("COUNT overflow");
    if ((other.sum_ > 0 && sum_ > std::numeric_limits<Value>::max() - other.sum_) ||
        (other.sum_ < 0 && sum_ < std::numeric_limits<Value>::min() - other.sum_))
      throw std::overflow_error("SUM overflow");
    rows_ += other.rows_;
    count_ += other.count_;
    sum_ += other.sum_;
    if (other.min_ && (!min_ || *other.min_ < *min_)) min_ = other.min_;
    if (other.max_ && (!max_ || *other.max_ > *max_)) max_ = other.max_;
  }
 private:
  std::uint64_t rows_ = 0, count_ = 0;
  Value sum_ = 0;
  std::optional<Value> min_, max_;
};
struct Group { Key key; State state; };

inline State global_aggregate(const std::vector<Row>& rows) {
  State state;
  for (const auto& row : rows) state.add(row.value);
  return state;
}

inline std::vector<Group> sort_aggregate(std::vector<Row> rows) {
  // ponytail: in-memory sort; replace with an external sorter when rows exceed RAM.
  std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.key < b.key; });
  std::vector<Group> groups;
  for (const auto& row : rows) {
    if (groups.empty() || groups.back().key != row.key) groups.push_back({row.key, {}});
    groups.back().state.add(row.value);
  }
  return groups;
}
} // namespace tutorial

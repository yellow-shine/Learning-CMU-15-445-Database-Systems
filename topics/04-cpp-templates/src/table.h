#pragma once
#include <utility>
#include <functional>
#include <vector>

// A bag of rows: duplicates and insertion order are preserved. Predicates
// observe const rows; results own copies, never iterators into this table.
template <class Row>
class Table {
 public:
  void insert(Row row) { rows_.push_back(std::move(row)); }
  template <class Predicate>
  std::vector<Row> select(Predicate predicate) const {
    std::vector<Row> result;
    for (const auto &row : rows_) if (std::invoke(predicate, row)) result.push_back(row);
    return result;
  }
  template <class Value, class Project>
  Value sum(Value initial, Project project) const {
    for (const auto &row : rows_) initial += std::invoke(project, row);
    return initial;
  }
  std::size_t size() const noexcept { return rows_.size(); }
 private:
  std::vector<Row> rows_;
};

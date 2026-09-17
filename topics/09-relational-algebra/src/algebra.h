#pragma once
#include "relation.h"
#include <functional>

namespace tutorial {
inline Relation select(const Relation &input, const std::function<bool(const Tuple &)> &predicate) {
  if (!predicate) throw std::invalid_argument("empty predicate");
  Relation result(input.schema());
  for (const auto &row : input.rows()) if (predicate(row)) result.insert(row);
  return result;
}
inline Relation project(const Relation &input, const std::vector<std::string> &names) {
  std::vector<std::size_t> columns;
  std::vector<Attribute> attributes;
  for (const auto &name : names) {
    const auto column = input.schema().index(name);
    columns.push_back(column);
    attributes.push_back(input.schema().attributes()[column]);
  }
  Relation result{Schema(attributes)};  // Also rejects repeated projection names.
  for (const auto &row : input.rows()) {
    Tuple projected;
    for (auto column : columns) projected.push_back(row[column]);
    result.insert(std::move(projected));
  }
  return result;
}
enum class SetOperation { Union, Intersection, Difference };
inline Relation combine(const Relation &left, const Relation &right, SetOperation operation) {
  if (!(left.schema() == right.schema())) throw std::invalid_argument("incompatible schemas");
  Relation result(left.schema());
  for (const auto &row : left.rows()) {
    const bool present = right.rows().count(row) != 0;
    if (operation == SetOperation::Union ||
        (operation == SetOperation::Intersection && present) ||
        (operation == SetOperation::Difference && !present)) result.insert(row);
  }
  if (operation == SetOperation::Union) for (const auto &row : right.rows()) result.insert(row);
  return result;
}
// Every column gets a side prefix, including the retained equijoin keys.
inline Schema paired_schema(const Schema &left, const Schema &right) {
  std::vector<Attribute> attributes;
  for (auto a : left.attributes()) { a.name = "L." + a.name; attributes.push_back(a); }
  for (auto a : right.attributes()) { a.name = "R." + a.name; attributes.push_back(a); }
  return Schema(std::move(attributes));
}
inline Tuple concatenate(Tuple left, const Tuple &right) {
  left.insert(left.end(), right.begin(), right.end());
  return left;
}
inline Relation product(const Relation &left, const Relation &right) {
  Relation result(paired_schema(left.schema(), right.schema()));
  for (const auto &l : left.rows()) for (const auto &r : right.rows()) result.insert(concatenate(l, r));
  return result;
}
inline Relation join(const Relation &left, const Relation &right,
                     const std::string &left_key, const std::string &right_key) {
  const auto li = left.schema().index(left_key), ri = right.schema().index(right_key);
  if (left.schema().attributes()[li].type != right.schema().attributes()[ri].type)
    throw std::invalid_argument("join key type mismatch");
  Relation result(paired_schema(left.schema(), right.schema()));
  // ponytail: nested loops are O(n*m); use a hash join when input size matters.
  for (const auto &l : left.rows()) for (const auto &r : right.rows())
    if (l[li] == r[ri]) result.insert(concatenate(l, r));
  return result;
}
}  // namespace tutorial

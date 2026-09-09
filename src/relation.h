#pragma once
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tutorial {
using Value = std::variant<std::monostate, std::int64_t, std::string>;
using Tuple = std::vector<Value>;
enum class Type { Integer, Text };
struct Attribute {
  std::string name;
  Type type;
  bool nullable = false;
  bool operator==(const Attribute &other) const {
    return name == other.name && type == other.type && nullable == other.nullable;
  }
};
class Schema {
 public:
  explicit Schema(std::vector<Attribute> attributes) : attributes_(std::move(attributes)) {
    std::set<std::string> names;
    for (const auto &a : attributes_) {
      if (a.name.empty() || !names.insert(a.name).second) {
        throw std::invalid_argument("empty or duplicate attribute name");
      }
    }
  }
  const std::vector<Attribute> &attributes() const { return attributes_; }
  std::size_t index(const std::string &name) const {
    for (std::size_t i = 0; i < attributes_.size(); ++i) {
      if (attributes_[i].name == name) return i;
    }
    throw std::invalid_argument("unknown attribute: " + name);
  }
  void validate(const Tuple &tuple) const {
    if (tuple.size() != attributes_.size()) throw std::invalid_argument("tuple arity");
    for (std::size_t i = 0; i < tuple.size(); ++i) {
      if (std::holds_alternative<std::monostate>(tuple[i])) {
        if (!attributes_[i].nullable) throw std::invalid_argument("NULL forbidden");
      } else if ((attributes_[i].type == Type::Integer && !std::holds_alternative<std::int64_t>(tuple[i])) ||
                 (attributes_[i].type == Type::Text && !std::holds_alternative<std::string>(tuple[i]))) {
        throw std::invalid_argument("value type mismatch");
      }
    }
  }
  bool operator==(const Schema &other) const { return attributes_ == other.attributes_; }
 private:
  std::vector<Attribute> attributes_;
};
// A relation is a set, not SQL's default bag. NULL here is a literal marker.
class Relation {
 public:
  explicit Relation(Schema schema) : schema_(std::move(schema)) {}
  bool insert(Tuple tuple) {
    schema_.validate(tuple);  // Validate the whole row before changing state.
    return rows_.insert(std::move(tuple)).second;
  }
  const Schema &schema() const { return schema_; }
  const std::set<Tuple> &rows() const { return rows_; }
 private:
  Schema schema_;
  std::set<Tuple> rows_;
};
inline std::string display(const Value &value) {
  if (std::holds_alternative<std::monostate>(value)) return "NULL";
  if (const auto *integer = std::get_if<std::int64_t>(&value)) return std::to_string(*integer);
  return std::get<std::string>(value);
}
}  // namespace tutorial

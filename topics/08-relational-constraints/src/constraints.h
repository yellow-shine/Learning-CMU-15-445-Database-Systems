#pragma once
#include "relation.h"
#include <algorithm>
#include <functional>

namespace tutorial {
struct ForeignKey {
  std::size_t column;
  std::string parent;
};
struct Table {
  std::string name;
  Schema schema;
  std::size_t primary_key;
  std::vector<ForeignKey> foreign_keys;
  std::vector<std::function<bool(const Tuple &)>> checks;
  std::vector<Tuple> rows;
};
class Database {
 public:
  // Targets must already exist: this tutorial intentionally admits only an acyclic schema.
  void create(std::string name, Schema schema, std::size_t primary_key,
              std::vector<ForeignKey> foreign_keys = {},
              std::vector<std::function<bool(const Tuple &)>> checks = {}) {
    if (name.empty()) throw std::invalid_argument("empty table name");
    for (const auto &t : tables_) if (t.name == name) throw std::invalid_argument("duplicate table");
    const auto &attributes = schema.attributes();
    if (primary_key >= attributes.size() || attributes[primary_key].nullable) {
      throw std::invalid_argument("primary key must be a NOT NULL column");
    }
    for (const auto &fk : foreign_keys) {
      const auto &parent = table(fk.parent);
      if (fk.column >= attributes.size() ||
          attributes[fk.column].type != parent.schema.attributes()[parent.primary_key].type) {
        throw std::invalid_argument("foreign key column/type");
      }
    }
    for (const auto &predicate : checks) if (!predicate) throw std::invalid_argument("empty check");
    tables_.push_back({std::move(name), std::move(schema), primary_key,
                       std::move(foreign_keys), std::move(checks), {}});
  }
  const Table &table(const std::string &name) const {
    return tables_[index(name)];
  }
  void insert(const std::string &name, Tuple row) {
    const auto i = index(name);
    tables_[i].schema.validate(row);
    // ponytail: copy the database for statement atomicity; use undo logs for large data.
    Database candidate = *this;
    candidate.tables_[i].rows.push_back(std::move(row));
    candidate.validate();
    tables_.swap(candidate.tables_);
  }
  bool erase(const std::string &name, const Value &key) {
    const auto i = index(name);
    const auto &t = tables_[i];
    Schema({t.schema.attributes()[t.primary_key]}).validate({key});
    Database candidate = *this;
    auto &rows = candidate.tables_[i].rows;
    const auto it = std::find_if(rows.begin(), rows.end(), [&](const Tuple &r) { return r[t.primary_key] == key; });
    if (it == rows.end()) return false;
    rows.erase(it);
    candidate.validate();  // RESTRICT: dangling references reject the whole delete.
    tables_.swap(candidate.tables_);
    return true;
  }
 private:
  std::vector<Table> tables_;
  std::size_t index(const std::string &name) const {
    for (std::size_t i = 0; i < tables_.size(); ++i) if (tables_[i].name == name) return i;
    throw std::invalid_argument("unknown table: " + name);
  }
  void validate() const {
    for (const auto &t : tables_) {
      std::set<Value> keys;
      for (const auto &row : t.rows) {
        t.schema.validate(row);
        if (!keys.insert(row[t.primary_key]).second) throw std::invalid_argument("duplicate primary key");
        for (const auto &predicate : t.checks) {
          if (!predicate(row)) throw std::invalid_argument("check constraint");
        }
        for (const auto &fk : t.foreign_keys) {
          const auto &value = row[fk.column];
          if (std::holds_alternative<std::monostate>(value)) continue;  // MATCH SIMPLE
          const auto &parent = table(fk.parent);
          if (std::none_of(parent.rows.begin(), parent.rows.end(), [&](const Tuple &p) {
                return p[parent.primary_key] == value;
              })) throw std::invalid_argument("missing foreign key");
        }
      }
    }
  }
};
}  // namespace tutorial

#include "constraints.h"
#include <iostream>
using namespace tutorial;
int main() {
  Database db;
  db.create("departments", Schema({{"id", Type::Integer}}), 0);
  db.create("people", Schema({{"id", Type::Integer}, {"dept", Type::Integer, true}, {"age", Type::Integer}}),
            0, {{1, "departments"}}, {[](const Tuple &r) { return std::get<std::int64_t>(r[2]) >= 0; }});
  db.insert("departments", {std::int64_t{10}});
  db.insert("people", {std::int64_t{1}, std::int64_t{10}, std::int64_t{20}});
  try { db.erase("departments", std::int64_t{10}); }
  catch (const std::invalid_argument &e) { std::cout << "delete rejected: " << e.what() << '\n'; }
  std::cout << "parents=" << db.table("departments").rows.size() << '\n';
  db.erase("people", std::int64_t{1});
  db.erase("departments", std::int64_t{10});
  std::cout << "after child-first delete: parents=" << db.table("departments").rows.size() << '\n';
}

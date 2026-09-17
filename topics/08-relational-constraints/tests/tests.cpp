#include "constraints.h"
#include "check.h"
#include <iostream>
using namespace tutorial;
Value number(std::int64_t n) { return n; }
int main() {
  try {
    Database db;
    db.create("parent", Schema({{"id", Type::Integer}}), 0);
    db.create("child", Schema({{"id", Type::Integer}, {"parent", Type::Integer, true}, {"age", Type::Integer}}),
              0, {{1, "parent"}}, {[](const Tuple &r) { return std::get<std::int64_t>(r[2]) >= 0; }});
    db.insert("parent", {number(7)});
    db.insert("child", {number(1), number(7), number(20)});
    const auto before = db.table("child").rows;
    rejects([&] { db.insert("child", {number(1), number(7), number(30)}); });
    rejects([&] { db.insert("child", {number(2), number(99), number(30)}); });
    rejects([&] { db.insert("child", {std::monostate{}, number(7), number(30)}); });
    rejects([&] { db.insert("child", {number(2), number(7), number(-1)}); });
    rejects([&] { db.insert("child", {number(2), number(7)}); });
    rejects([&] { db.insert("child", {number(2), std::string{"7"}, number(2)}); });
    check(db.table("child").rows == before, "all failed inserts atomic");
    rejects([&] { db.erase("parent", number(7)); });
    check(db.table("parent").rows.size() == 1, "restricted delete atomic");
    db.insert("child", {number(2), std::monostate{}, number(0)});
    check(!db.erase("child", number(999)), "missing delete");
    rejects([&] { db.erase("child", std::string{"1"}); });
    check(db.erase("child", number(1)), "child delete");
    check(db.erase("parent", number(7)), "parent now deletable");
    check(db.table("child").rows.size() == 1, "NULL foreign key exempt");
    rejects([&] { db.create("parent", Schema({{"id", Type::Integer}}), 0); });
    rejects([&] { db.create("bad", Schema({{"id", Type::Integer, true}}), 0); });
    rejects([&] { db.create("bad", Schema({{"id", Type::Integer}}), 1); });
    rejects([&] { db.create("bad", Schema({{"id", Type::Integer}}), 0, {{1, "parent"}}); });
    rejects([&] { db.create("bad", Schema({{"id", Type::Text}}), 0, {{0, "parent"}}); });
    rejects([&] { db.create("bad", Schema({{"id", Type::Integer}}), 0, {{0, "missing"}}); });
    rejects([&] { db.create("bad", Schema({{"id", Type::Integer}}), 0, {}, { { } }); });
    rejects([&] { db.insert("unknown", {}); });
    db.create("throws", Schema({{"id", Type::Integer}}), 0, {},
              {[](const Tuple &) -> bool { throw std::runtime_error("predicate exception"); }});
    bool threw = false;
    try { db.insert("throws", {number(1)}); } catch (const std::runtime_error &) { threw = true; }
    check(threw && db.table("throws").rows.empty(), "throwing check atomic");
    std::cout << "relational-constraints tests passed\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}

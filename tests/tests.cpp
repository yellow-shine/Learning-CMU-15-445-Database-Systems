#include "algebra.h"
#include "check.h"
#include <iostream>
using namespace tutorial;
Value n(std::int64_t i) { return i; }
int main() {
  try {
    Schema s({{"id", Type::Integer}, {"group", Type::Text}});
    Relation a(s), b(s), empty(s);
    a.insert({n(1), std::string{"x"}}); a.insert({n(2), std::string{"x"}});
    check(!a.insert({n(1), std::string{"x"}}), "duplicate input");
    b.insert({n(2), std::string{"x"}}); b.insert({n(3), std::string{"y"}});
    check(select(a, [](const Tuple &r) { return r[0] == n(2); }).rows() == std::set<Tuple>{{n(2), std::string{"x"}}}, "selection");
    check(select(a, [](const Tuple &) { return false; }).rows().empty(), "false selection");
    check(project(a, {"group"}).rows() == std::set<Tuple>{{std::string{"x"}}}, "projection dedup");
    check(project(a, {}).rows().size() == 1 && project(empty, {}).rows().empty(), "zero degree projection");
    check(project(a, {"group", "id"}).schema().attributes()[0].name == "group", "projection order");
    rejects([&] { project(a, {"missing"}); });
    rejects([&] { project(a, {"id", "id"}); });
    rejects([&] { select(a, {}); });
    check(combine(a, b, SetOperation::Union).rows().size() == 3, "union");
    check(combine(a, b, SetOperation::Intersection).rows() == std::set<Tuple>{{n(2), std::string{"x"}}}, "intersection");
    check(combine(a, b, SetOperation::Difference).rows() == std::set<Tuple>{{n(1), std::string{"x"}}}, "difference");
    check(combine(a, a, SetOperation::Difference).rows().empty(), "self difference");
    for (auto op : {SetOperation::Union, SetOperation::Intersection, SetOperation::Difference}) {
      check(combine(empty, empty, op).rows().empty(), "empty sets");
      for (auto bad : {Schema({{"other", Type::Integer}, {"group", Type::Text}}),
                       Schema({{"id", Type::Text}, {"group", Type::Text}}),
                       Schema({{"id", Type::Integer, true}, {"group", Type::Text}}), Schema({})})
        rejects([&] { combine(a, Relation(bad), op); });
    }
    auto p = product(a, b), j = join(a, b, "group", "group");
    check(p.rows().size() == 4 && j.rows().size() == 2, "product and equijoin");
    check(j.schema().index("L.group") == 1 && j.schema().index("R.group") == 3, "both join keys retained");
    check(j.rows() == select(p, [](const Tuple &r) { return r[1] == r[3]; }).rows(), "join equals product selection");
    check(product(empty, a).rows().empty() && join(a, empty, "id", "id").rows().empty(), "empty pair");
    rejects([&] { join(a, b, "id", "group"); });
    rejects([&] { join(empty, empty, "bad", "id"); });
    Relation nullable(Schema({{"v", Type::Integer, true}})); nullable.insert({std::monostate{}});
    check(join(nullable, nullable, "v", "v").rows().size() == 1, "literal NULL equality, not SQL");
    Relation unit(Schema({})); unit.insert({});
    check(product(unit, a).rows() == a.rows(), "zero degree identity");
    check(a.rows().size() == 2 && b.rows().size() == 2, "inputs unchanged");
    std::cout << "relational-algebra tests passed\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}

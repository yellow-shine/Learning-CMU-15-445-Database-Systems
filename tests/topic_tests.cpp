#include "engine.h"
#include <iostream>
#include <stdexcept>
#define CHECK(...) do { if (!(__VA_ARGS__)) throw std::runtime_error("check failed: " #__VA_ARGS__); } while(false)
int main() { try {

ModificationTable table;
CHECK(table.Insert({{1,10},{2,20},{3,30}})==std::vector<std::size_t>({0,1,2})); CHECK(table.Consistent());
auto unchanged=[&](auto operation) {
    const auto before=table.rows(); bool threw=false;
    try { operation(); } catch(const std::exception&) { threw=true; }
    CHECK(threw && table.rows()==before && table.Consistent());
    for(std::size_t i=0;i<before.size();++i) if(before[i]) CHECK(table.Find(before[i]->key)==std::optional<std::size_t>(i));
};
unchanged([&]{table.Insert({{4,40},{2,99}});});
unchanged([&]{table.Insert({{4,40},{4,99}});});
unchanged([&]{table.Update({{0,{4,1}},{1,{3,2}}});});
unchanged([&]{table.Update({{0,{4,1}},{999,{5,2}}});});
unchanged([&]{table.Update({{0,{4,1}},{0,{5,2}}});});
unchanged([&]{table.Delete({0,999});}); unchanged([&]{table.Delete({0,0});});
for(auto fail:{FailPoint::AfterTable,FailPoint::DuringIndex}) {
    unchanged([&]{table.Insert({{4,40}},fail);});
    unchanged([&]{table.Update({{0,{4,40}}},fail);});
    unchanged([&]{table.Delete({0},fail);});
}
table.Update({{0,{2,11}},{1,{1,22}}}); CHECK(table.Find(2)==std::optional<std::size_t>(0)); CHECK(table.Find(1)==std::optional<std::size_t>(1));
table.Delete({1}); CHECK(!table.Find(1) && table.Consistent()); unchanged([&]{table.Update({{1,{9,9}}});});
CHECK(table.Insert({{1,99}})==std::vector<std::size_t>({3})); CHECK(table.Consistent());
table.Delete({0,2,3}); CHECK(table.Consistent() && !table.Find(1));
table.Insert({}); table.Update({}); table.Delete({}); CHECK(table.Consistent());

std::cout << "all checks passed\n";
} catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

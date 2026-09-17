#include "tutorial.hpp"
#include <iostream>
using namespace tutorial;
void check(bool ok) { if (!ok) throw std::runtime_error("check failed"); }
template<class F> void rejects(F f) { bool caught=false; try { f(); } catch(const std::exception&) { caught=true; } check(caught); }
int main() { try {

check(serial_order(precedence({}))->empty());
Schedule good={{1,"x",Kind::Write},{2,"x",Kind::Read},{2,"y",Kind::Write},{3,"y",Kind::Read}};
auto g=precedence(good); check(g.at(1)==std::set<int>{2});
check(*serial_order(g)==std::vector<int>({1,2,3}));
Schedule bad={{1,"x",Kind::Read},{2,"x",Kind::Write},{2,"y",Kind::Read},{1,"y",Kind::Write}};
check(!serial_order(precedence(bad)));
check(precedence({{1,"x",Kind::Read},{2,"x",Kind::Read}}).at(1).empty());
check(precedence({{1,"x",Kind::Write},{1,"x",Kind::Read}}).at(1).empty());
check(precedence({{1,"x",Kind::Write},{2,"z",Kind::Write}}).at(1).empty());
check(!serial_order(Graph{{1,{1}}}));
check(*serial_order(Graph{{1,{2}}})==std::vector<int>({1,2}));
rejects([]{precedence({{-1,"x",Kind::Read}});});
rejects([]{precedence({{1,"",Kind::Write}});});
// Exhaust all length-four schedules over two transactions, keys and operation kinds.
for (int bits=0; bits<4096; ++bits) {
  int n=bits; Schedule s;
  for(int i=0;i<4;++i) { int op=n%8; n/=8; s.push_back({op%2,std::string(1,char('x'+(op/2)%2)),op/4 ? Kind::Write:Kind::Read}); }
  auto graph=precedence(s); bool forward=graph[0].count(1), backward=graph[1].count(0);
  check(serial_order(graph).has_value()==!(forward && backward));
}

std::cout << "all checks passed\n"; return 0;
} catch(const std::exception& e) { std::cerr << e.what() << "\n"; return 1; } }

#include "tutorial.hpp"
#include <iostream>
using namespace tutorial;
void check(bool ok) { if (!ok) throw std::runtime_error("check failed"); }
template<class F> void rejects(F f) { bool caught=false; try { f(); } catch(const std::exception&) { caught=true; } check(caught); }
int main() { try {

Deadlocks d; for(int i=1;i<=4;++i)d.begin(i);
check(d.cycle().empty()); check(!d.resolve_one());
check(d.request(1,"a")); check(d.request(2,"b")); check(d.request(3,"c"));
check(!d.request(1,"b")); check(!d.request(2,"c")); check(!d.request(3,"a"));
check(!d.request(4,"a"));
check(d.cycle().size()==3); check(d.resolve_one()==3);
check(d.state(3)==Deadlocks::State::Aborted); check(d.cycle().empty());
check(d.request(2,"c")); d.commit(2); check(d.request(1,"b")); d.commit(1);
check(d.request(4,"a")); d.commit(4); check(d.waits_for().empty());
rejects([&]{d.request(3,"c");}); rejects([&]{d.begin(3);});
Deadlocks chain; chain.begin(1); chain.begin(2); check(chain.request(1,"x"));
check(!chain.request(2,"x")); check(chain.cycle().empty());
rejects([&]{chain.request(2,"y");}); rejects([&]{chain.commit(2);});
chain.abort(1); check(chain.waits_for().empty()); check(chain.request(2,"x"));
check(chain.request(2,"x")); chain.abort(2);
// Two disjoint cycles require two resolutions.
Deadlocks two; for(int i=0;i<4;++i){two.begin(i); check(two.request(i,std::to_string(i)));}
for(int i=0;i<4;++i) check(!two.request(i,std::to_string(i^1)));
check(two.resolve_one()==1); check(two.resolve_one()==3); check(!two.resolve_one());

std::cout << "all checks passed\n"; return 0;
} catch(const std::exception& e) { std::cerr << e.what() << "\n"; return 1; } }

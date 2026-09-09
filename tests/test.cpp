#include "tutorial.hpp"
#include <iostream>
using namespace tutorial;
void check(bool ok) { if (!ok) throw std::runtime_error("check failed"); }
template<class F> void rejects(F f) { bool caught=false; try { f(); } catch(const std::exception&) { caught=true; } check(caught); }
int main() { try {

auto examples=anomalies();check(examples.size()==5);
check(examples[0].first==9 && examples[0].second==0);
check(examples[1].first==0 && examples[1].second==1);
check(examples[2].first==1 && examples[2].second==2);
check(examples[3].first==2 && examples[3].second==1);
check(examples[4].first==2 && examples[4].second==0);
for(auto level:{Level::ReadCommitted,Level::Snapshot}) {
 Isolation d(level,{{"x",0}});int a=d.begin(),b=d.begin();d.write(a,"x",9);check(d.read(b,"x")==0);d.abort(a);d.commit(b);
}
Isolation rr(Level::RepeatableRead,{{"x",0}});int a=rr.begin(),b=rr.begin();check(rr.read(a,"x")==0);
try {rr.write(b,"x",1);check(false);}catch(const WouldBlock&){}
check(rr.read(a,"x")==0);rr.commit(a);rr.write(b,"x",1);check(rr.commit(b));
Isolation si(Level::Snapshot,{{"x",0}});a=si.begin();b=si.begin();auto before=si.scan(a);
si.write(b,"x",1);si.write(b,"new",2);check(si.commit(b));check(si.scan(a)==before);
si.write(a,"x",2);check(!si.commit(a));rejects([&]{si.read(a,"x");});
Isolation skew(Level::RepeatableRead,{{"a",1},{"b",1}});a=skew.begin();b=skew.begin();skew.read(a,"b");skew.read(b,"a");
try{skew.write(a,"a",0);check(false);}catch(const WouldBlock&){}
try{skew.write(b,"b",0);check(false);}catch(const WouldBlock&){}
skew.abort(b);skew.write(a,"a",0);check(skew.commit(a));
Isolation empty(Level::Snapshot,{});a=empty.begin();check(empty.scan(a).empty());rejects([&]{empty.write(a,"",1);});empty.abort(a);

std::cout << "all checks passed\n"; return 0;
} catch(const std::exception& e) { std::cerr << e.what() << "\n"; return 1; } }

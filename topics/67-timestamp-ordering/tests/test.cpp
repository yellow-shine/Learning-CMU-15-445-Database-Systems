#include "tutorial.hpp"
#include <iostream>
using namespace tutorial;
void check(bool ok) { if (!ok) throw std::runtime_error("check failed"); }
template<class F> void rejects(F f) { bool caught=false; try { f(); } catch(const std::exception&) { caught=true; } check(caught); }
int main() { try {

TimestampOrdering db({{"x",0},{"y",0}});
auto old=db.begin(), young=db.begin(); check(db.read(young,"x")==0);
check(!db.write(old,"x",1)); rejects([&]{db.commit(old);}); check(db.commit(young));
auto a=db.begin(), b=db.begin(); check(db.write(b,"x",2)); check(db.commit(b));
check(!db.read(a,"x"));
auto c=db.begin(), d=db.begin(); check(db.write(c,"y",5)); check(db.write(c,"x",5));
check(db.read(c,"y")==5); check(db.read(d,"x")==2); check(!db.commit(c)); check(db.commit(d));
auto e=db.begin(); check(db.read(e,"y")==0); check(db.read(e,"x")==2); check(db.commit(e));
auto f=db.begin(), g=db.begin(); check(db.write(f,"y",9)); check(db.read(g,"y")==0);
db.abort(f); check(db.commit(g));
auto h=db.begin(), i=db.begin(); check(db.write(i,"y",7)); check(db.commit(i)); check(!db.write(h,"y",8));
auto j=db.begin(); rejects([&]{db.read(j,"missing");}); db.abort(j);
TimestampOrdering empty({}); check(empty.commit(empty.begin()));

std::cout << "all checks passed\n"; return 0;
} catch(const std::exception& e) { std::cerr << e.what() << "\n"; return 1; } }

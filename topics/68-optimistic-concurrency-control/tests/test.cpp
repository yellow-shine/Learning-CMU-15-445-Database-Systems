#include "tutorial.hpp"
#include <iostream>
using namespace tutorial;
void check(bool ok) { if (!ok) throw std::runtime_error("check failed"); }
template<class F> void rejects(F f) { bool caught=false; try { f(); } catch(const std::exception&) { caught=true; } check(caught); }
int main() { try {

OCC db({{"x",10},{"y",20}}); int a=db.begin(), b=db.begin();
check(db.read(a,"x")==10); check(db.read(b,"x")==10);
db.write(a,"x",11); db.write(b,"x",12); check(db.commit(a)); check(!db.commit(b));
rejects([&]{db.read(b,"x");});
a=db.begin(); b=db.begin(); db.write(a,"x",30); db.write(b,"y",40);
check(db.commit(a)); check(db.commit(b));
a=db.begin(); b=db.begin(); check(db.read(a,"x")==30); db.write(b,"x",31); check(db.commit(b));
check(db.read(a,"x")==30); check(!db.commit(a));
a=db.begin(); db.write(a,"x",99); check(db.read(a,"x")==99); db.abort(a);
b=db.begin(); check(db.read(b,"x")==31); check(db.commit(b));
// Write skew: both read x,y; only one may commit despite distinct write keys.
a=db.begin(); b=db.begin(); db.read(a,"x"); db.read(a,"y"); db.read(b,"x"); db.read(b,"y");
db.write(a,"x",0); db.write(b,"y",0); check(db.commit(a)); check(!db.commit(b));
// A failed multi-key commit publishes neither key.
a=db.begin(); b=db.begin(); db.write(a,"x",7); db.write(a,"y",7); db.write(b,"y",8);
check(db.commit(b)); check(!db.commit(a)); int c=db.begin(); check(db.read(c,"x")==0); check(db.read(c,"y")==8); db.abort(c);
a=db.begin(); rejects([&]{db.read(a,"missing");}); db.abort(a);
OCC empty({}); check(empty.commit(empty.begin()));

std::cout << "all checks passed\n"; return 0;
} catch(const std::exception& e) { std::cerr << e.what() << "\n"; return 1; } }

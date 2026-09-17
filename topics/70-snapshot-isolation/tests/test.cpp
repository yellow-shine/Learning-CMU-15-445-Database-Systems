#include "tutorial.hpp"
#include <iostream>
using namespace tutorial;
void check(bool ok) { if (!ok) throw std::runtime_error("check failed"); }
template<class F> void rejects(F f) { bool caught=false; try { f(); } catch(const std::exception&) { caught=true; } check(caught); }
int main() { try {

MVCC db({{"x",10},{"y",20}}); int a=db.begin(), b=db.begin();
check(db.read(a,"x")==10); db.write(b,"x",11); check(db.commit(b));
check(db.read(a,"x")==10); check(db.scan(a).at("x")==10);
db.write(a,"x",12); db.write(a,"y",21); check(!db.commit(a));
int c=db.begin(); check(db.read(c,"x")==11); check(db.read(c,"y")==20); db.commit(c);
a=db.begin(); b=db.begin(); auto before=db.scan(a); db.write(b,"z",30); db.write(b,"y",std::nullopt); check(db.commit(b));
check(db.scan(a)==before); db.write(a,"own",40); db.write(a,"y",std::nullopt);
check(db.scan(a).count("own")==1); check(db.scan(a).count("y")==0); db.abort(a);
// Distinct writes survive; SI permits write skew.
MVCC doctors({{"alice",1},{"bob",1}}); a=doctors.begin(); b=doctors.begin();
check(doctors.read(a,"bob")==1); check(doctors.read(b,"alice")==1);
doctors.write(a,"alice",0); doctors.write(b,"bob",0); check(doctors.commit(a)); check(doctors.commit(b));
c=doctors.begin(); check(doctors.read(c,"alice")==0 && doctors.read(c,"bob")==0); doctors.commit(c);
MVCC inserts({}); a=inserts.begin(); b=inserts.begin(); inserts.write(a,"same",1); inserts.write(b,"same",2);
check(inserts.commit(b)); check(!inserts.commit(a));
a=inserts.begin(); b=inserts.begin(); inserts.write(a,"same",std::nullopt); inserts.write(b,"same",5);
check(inserts.commit(a)); check(!inserts.commit(b));
c=inserts.begin(); check(inserts.scan(c).empty()); check(inserts.commit(c)); rejects([&]{inserts.scan(c);});

std::cout << "all checks passed\n"; return 0;
} catch(const std::exception& e) { std::cerr << e.what() << "\n"; return 1; } }

#include "tutorial.hpp"
#include <iostream>
using namespace tutorial;
void check(bool ok) { if (!ok) throw std::runtime_error("check failed"); }
template<class F> void rejects(F f) { bool caught=false; try { f(); } catch(const std::exception&) { caught=true; } check(caught); }
int main() { try {

MVCC db({{"x",10}}); int oldest=db.begin(), writer=db.begin();
db.write(writer,"x",20); check(db.read(oldest,"x")==10); check(db.read(writer,"x")==20); check(db.commit(writer));
int middle=db.begin(); writer=db.begin(); db.write(writer,"x",30); check(db.commit(writer));
int newest=db.begin(); check(db.read(oldest,"x")==10); check(db.read(middle,"x")==20); check(db.read(newest,"x")==30);
writer=db.begin(); db.write(writer,"x",std::nullopt); check(db.commit(writer));
int deleted=db.begin(); check(!db.read(deleted,"x")); check(db.read(newest,"x")==30);
writer=db.begin(); db.write(writer,"x",40); db.write(writer,"new",7); check(db.commit(writer));
check(!db.read(deleted,"new")); check(!db.read(oldest,"new"));
int current=db.begin(); check(db.read(current,"x")==40); check(db.read(current,"new")==7);
writer=db.begin(); db.write(writer,"x",50); db.abort(writer); check(db.read(current,"x")==40);
rejects([&]{db.read(writer,"x");});
// Old writers cannot overwrite a chain updated after their snapshot.
db.write(oldest,"x",99); check(!db.commit(oldest));
for(int id:{middle,newest,deleted,current}) check(db.commit(id));
MVCC empty({}); int a=empty.begin(), b=empty.begin(); empty.write(a,"k",1); empty.write(b,"k",2);
check(empty.commit(a)); check(!empty.commit(b));
int c=empty.begin(); rejects([&]{empty.write(c,"",1);}); empty.abort(c);
Row row{3,30,{{0,10},{1,20}}}; check(row.at(0)==10); check(row.at(1)==20); check(row.at(2)==20); check(row.at(3)==30);

std::cout << "all checks passed\n"; return 0;
} catch(const std::exception& e) { std::cerr << e.what() << "\n"; return 1; } }

#include "tutorial.hpp"
#include <iostream>
using namespace tutorial;
void check(bool ok) { if (!ok) throw std::runtime_error("check failed"); }
template<class F> void rejects(F f) { bool caught=false; try { f(); } catch(const std::exception&) { caught=true; } check(caught); }
int main() { try {

MVCC doctors({{"alice",1},{"bob",1}});int a=doctors.begin(),b=doctors.begin();
check(doctors.read(a,"bob")==1);check(doctors.read(b,"alice")==1);
doctors.write(a,"alice",0);doctors.write(b,"bob",0);check(doctors.commit(a));check(!doctors.commit(b));
int c=doctors.begin();check(doctors.read(c,"alice")==0 && doctors.read(c,"bob")==1);check(doctors.commit(c));
// Phantom-type conflict: a scan deciding whether to insert must validate its predicate.
MVCC bookings({});a=bookings.begin();b=bookings.begin();check(bookings.scan(a).empty());check(bookings.scan(b).empty());
bookings.write(a,"slot-a",1);bookings.write(b,"slot-b",1);check(bookings.commit(a));check(!bookings.commit(b));
c=bookings.begin();check(bookings.scan(c).size()==1);check(bookings.commit(c));
// Missing-key reads also conflict with later insertions.
a=bookings.begin();b=bookings.begin();check(!bookings.read(a,"missing"));bookings.write(b,"missing",3);check(bookings.commit(b));
bookings.write(a,"different",9);check(!bookings.commit(a));
c=bookings.begin();check(!bookings.read(c,"different"));bookings.abort(c);
// Unrelated point writes succeed, unlike conservative table scans.
a=bookings.begin();b=bookings.begin();check(bookings.read(a,"slot-a")==1);bookings.write(b,"other",5);check(bookings.commit(b));check(bookings.commit(a));
a=bookings.begin();b=bookings.begin();bookings.scan(a);bookings.write(b,"unrelated",0);check(bookings.commit(b));check(!bookings.commit(a));
// Stable old reads are not enough to commit after deletion.
a=bookings.begin();b=bookings.begin();check(bookings.read(a,"slot-a")==1);bookings.write(b,"slot-a",std::nullopt);check(bookings.commit(b));
check(bookings.read(a,"slot-a")==1);check(!bookings.commit(a));
// Same-key blind write conflict and own-write overlay.
a=bookings.begin();b=bookings.begin();bookings.write(a,"k",1);bookings.write(b,"k",2);
check(bookings.read(a,"k")==1);check(bookings.scan(a).at("k")==1);check(bookings.commit(a));check(!bookings.commit(b));
rejects([&]{bookings.commit(b);});
MVCC empty({});a=empty.begin();check(empty.scan(a).empty());check(empty.commit(a));

std::cout << "all checks passed\n"; return 0;
} catch(const std::exception& e) { std::cerr << e.what() << "\n"; return 1; } }

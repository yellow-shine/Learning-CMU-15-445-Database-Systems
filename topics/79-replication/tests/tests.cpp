#include "replication.hpp"
#include "check.hpp"
int main() {
 using namespace tutorial;
 Replication r; CHECK(!r.deliver_apply() && !r.deliver_ack());
 rejects([&]{r.acknowledged(0);}); rejects([&]{r.acknowledged(1);});
 auto a=r.write(7,Mode::Async); CHECK(r.acknowledged(a)); CHECK(r.lag()==1); CHECK(r.read_replica().empty());
 r.set_link(false); CHECK(!r.deliver_apply()); r.set_link(true);
 r.set_replica_alive(false); CHECK(!r.deliver_apply()); rejects([&]{r.read_replica();});
 r.set_replica_alive(true); CHECK(r.deliver_apply()); CHECK(r.lag()==0); CHECK(r.deliver_ack());
 auto s=r.write(9,Mode::Sync); CHECK(!r.acknowledged(s)); CHECK(r.deliver_apply());
 CHECK(!r.acknowledged(s)); CHECK(r.read_replica()==std::vector<int>({7,9}));
 r.set_link(false); CHECK(!r.deliver_ack()); CHECK(!r.acknowledged(s));
 r.set_link(true); CHECK(r.deliver_ack()); CHECK(r.acknowledged(s));
 Replication lost; auto l=lost.write(42,Mode::Async); CHECK(lost.acknowledged(l));
 lost.fail_primary(); CHECK(!lost.deliver_apply()); CHECK(lost.read_replica().empty()); rejects([&]{lost.write(1,Mode::Sync);});
 Replication uncertain; auto u=uncertain.write(8,Mode::Sync); uncertain.deliver_apply(); uncertain.fail_primary();
 CHECK(!uncertain.acknowledged(u)); CHECK(uncertain.read_replica()==std::vector<int>({8}));
 for(int i=0;i<100;++i) {auto n=r.write(i,Mode::Sync); r.deliver_apply(); r.deliver_ack(); CHECK(r.acknowledged(n));}
 CHECK(r.lag()==0);
}

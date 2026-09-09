#include "replication.hpp"
#include <iostream>
int main() {
 tutorial::Replication r;
 auto a=r.write(7,tutorial::Mode::Async);
 std::cout<<"async ack="<<r.acknowledged(a)<<" lag="<<r.lag()<<'\n';
 r.deliver_apply(); r.deliver_ack();
 auto s=r.write(9,tutorial::Mode::Sync);
 std::cout<<"sync before="<<r.acknowledged(s)<<'\n';
 r.deliver_apply();
 std::cout<<"after apply="<<r.acknowledged(s)<<" lag="<<r.lag()<<'\n';
 r.deliver_ack(); std::cout<<"after ack="<<r.acknowledged(s)<<'\n';
}

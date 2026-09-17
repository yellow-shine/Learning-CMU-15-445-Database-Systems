#include "distributed_query.hpp"
#include <iostream>
int main() {
 using namespace tutorial;
 Rows l{{1,10},{1,11},{2,20}},r{{1,100},{1,101},{3,300}};
 auto b=broadcast_join(l,r,2),h=repartition_join(l,r,2);
 std::cout<<"broadcast rows="<<b.rows.size()<<" sent="<<b.sent<<'\n';
 std::cout<<"repartition rows="<<h.rows.size()<<" sent="<<h.sent<<'\n';
 auto a=partial_aggregate({{{1,10}},{{1,11},{2,20}}},2); auto g=a.groups.at(1);
 std::cout<<"key=1 sum="<<g.sum<<" count="<<g.count<<" avg="<<g.average()<<" partials="<<a.sent<<'\n';
}

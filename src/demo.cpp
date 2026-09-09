#include "aggregate.hpp"
#include <iostream>
int main(){using namespace db;
 auto p=node(Kind::Aggregate,node(Kind::Join,facts({{1,10},{1,20},{2,90}}),dimension({1,1,2})));
 auto q=push_aggregate(p);require(equivalent(p,q));
 for(auto g:execute(q))std::cout<<"key="<<g.first<<" sum="<<g.second.sum<<" count="<<g.second.count<<" avg="<<*g.second.average()<<'\n';
}

#include "ordering.hpp"
#include <iostream>
int main(){using namespace db;Query q{{{0,1,0,1,0,1,0,1},{0,1},{0}}};auto p=dynamic_program(q);auto plans=exhaustive(q);
 auto optimal=(*std::min_element(plans.begin(),plans.end(),[](P a,P b){return a->cost<b->cost;}))->cost;
 std::size_t comparisons=0;auto rows=execute(q,p,comparisons);require(p->cost==optimal&&result(q,p)==result(q,left_deep(q)));
 std::cout<<describe(p)<<"\ndp="<<p->cost<<" exhaustive="<<optimal<<" left-deep="<<left_deep(q)->cost<<" actual comparisons="<<comparisons<<" rows="<<rows.size()<<'\n';
}

#include "optimizer.hpp"
#include <iostream>
int main(){using namespace db;
 auto s=scan({"x","unused"},{{{"x",3},{"unused",0}},{{"x",1},{"unused",0}},{{"x",2},{"unused",0}}});
 auto p=limit(project(s,{"x"}),2);auto q=push_limit(p);require(execute(p)==execute(q));
 std::cout<<"safe prefix:";for(auto&r:execute(q))std::cout<<' '<<*r.at("x");
 auto sorted=unary(Kind::Sort,s);sorted->leftKey="x";auto top=limit(sorted,1);require(push_limit(top)==top);
 std::cout<<"; sorted first="<<*execute(top)[0].at("x")<<'\n';
}

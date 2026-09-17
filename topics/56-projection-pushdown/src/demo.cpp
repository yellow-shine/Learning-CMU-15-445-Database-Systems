#include "optimizer.hpp"
#include <iostream>
int main(){using namespace db;
 auto a=scan({"a","v","unused"},{{{"a",1},{"v",10},{"unused",99}},{{"a",1},{"v",20},{"unused",88}}});
 auto g=unary(Kind::Aggregate,filter(a,{"v",5}));g->leftKey="a";g->rightKey="v";
 auto p=project(g,{"sum"});auto q=prune(p,{"sum"});require(equivalent(p,q));
 std::cout<<"sum="<<*execute(q).at(0).at("sum")<<" scan columns=3 required=2\n";
}

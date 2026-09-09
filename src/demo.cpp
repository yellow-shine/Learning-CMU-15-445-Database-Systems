#include "optimizer.hpp"
#include <iostream>
int main(){using namespace db;
 auto a=scan({"a"},{{{"a",1}},{{"a",2}},{{"a",2}}});auto b=scan({"b"},{{{"b",2}},{{"b",2}},{{"b",3}}});
 for(auto cmp:{Compare::Equal,Compare::Less}){auto p=join(a,b,"a","b",Kind::Join,cmp);auto q=select_physical(p);require(equivalent(p,q));std::cout<<implementation(q)<<" rows="<<execute(q).size()<<'\n';}
}

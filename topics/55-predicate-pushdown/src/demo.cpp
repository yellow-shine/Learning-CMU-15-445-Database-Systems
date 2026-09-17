#include "optimizer.hpp"
#include <iostream>
int main(){using namespace db;
 auto a=scan({"a.k","a.v"},{{{"a.k",1},{"a.v",5}},{{"a.k",2},{"a.v",20}}});
 auto b=scan({"b.k"},{{{"b.k",1}},{{"b.k",2}}});
 auto p=filter(join(a,b,"a.k","b.k"),{"a.v",10});auto q=push_predicate(p);
 require(equivalent(p,q));std::cout<<"before="<<execute(p).size()<<" after="<<execute(q).size()<<" left input="<<execute(q->left).size()<<'\n';
}

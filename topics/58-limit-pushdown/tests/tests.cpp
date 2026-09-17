#include "optimizer.hpp"
int main(){using namespace db;
 auto s=scan({"x","v"},{{{"x",3},{"v",0}},{{"x",1},{"v",0}},{{"x",2},{"v",0}},{{"x",2},{"v",1}}});
 for(std::size_t n:{0,1,2,8}){auto p=limit(project(project(s,{"x","v"}),{"x"}),n);auto q=push_limit(p);require(q->kind==Kind::Project&&execute(p)==execute(q));
 for(std::size_t m:{0,1,4,9}){auto nested=limit(limit(s,m),n);require(execute(nested)==execute(push_limit(nested)));}}
 auto filtered=limit(filter(s,{"x",2}),2);require(push_limit(filtered)==filtered);
 // A qualifying row beyond the prefix is lost by premature LIMIT.
 auto f=limit(filter(s,{"x",1}),3);auto wrong=filter(limit(s,3),{"x",1});require(execute(f)!=execute(wrong));
 auto sorted=unary(Kind::Sort,s);sorted->leftKey="x";auto p=limit(sorted,1);require(push_limit(p)==p);
 auto badsort=unary(Kind::Sort,limit(s,1));badsort->leftKey="x";require(execute(p)!=execute(badsort));
 auto b=scan({"b"},{{{"b",2}}});auto j=limit(join(s,b,"x","b"),1);require(push_limit(j)==j);require(execute(j)!=execute(join(limit(s,1),b,"x","b")));
 auto empty=limit(project(scan({"x"},{}),{"x"}),0);require(execute(push_limit(empty)).empty());
}

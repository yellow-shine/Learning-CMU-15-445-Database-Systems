#include "optimizer.hpp"
int main(){using namespace db;
 auto a=scan({"a"},{{{"a",1}},{{"a",2}},{{"a",2}},{{"a",std::nullopt}}});
 auto b=scan({"b"},{{{"b",1}},{{"b",2}},{{"b",2}}});
 for(auto col:{"a","b"}){auto p=filter(join(a,b,"a","b"),{col,1});auto q=push_predicate(p);require(q->kind==Kind::Join&&equivalent(p,q));require(execute(q).size()==4);}
 auto outer=filter(join(a,b,"a","b",Kind::LeftJoin),{"b",1});require(push_predicate(outer)==outer);
 auto wrong=join(a,filter(b,{"b",1}),"a","b",Kind::LeftJoin);require(!equivalent(outer,wrong));
 auto empty=filter(join(scan({"a"},{}),b,"a","b"),{"a",0});require(equivalent(empty,push_predicate(empty)));
 bool rejected=false;try{filter(a,{"missing",1});}catch(const std::invalid_argument&){rejected=true;}require(rejected);
 for(int t=-1;t<4;++t){auto p=filter(join(a,b,"a","b"),{"a",t});require(equivalent(p,push_predicate(p)));}
}

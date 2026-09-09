#include "optimizer.hpp"
int main(){using namespace db;
 for(int n=0;n<7;++n){Rows x,y;for(int i=0;i<n;++i)x.push_back({{"a",i%3}});for(int i=0;i<5;++i)y.push_back({{"b",i%3}});
 x.push_back({{"a",std::nullopt}});y.push_back({{"b",std::nullopt}});
 auto a=scan({"a"},x),b=scan({"b"},y);
 for(auto cmp:{Compare::Equal,Compare::Less,Compare::Greater}){auto p=join(a,b,"a","b",Kind::Join,cmp);auto q=select_physical(p);require(equivalent(p,q));require((q->kind==Kind::HashJoin)==(cmp==Compare::Equal));}
 auto outer=join(a,b,"a","b",Kind::LeftJoin);auto q=select_physical(outer);require(q->kind==Kind::LeftJoin&&equivalent(outer,q));
 auto wrapped=filter(join(a,b,"a","b"),{"a",0});require(select_physical(wrapped)->left->kind==Kind::HashJoin&&equivalent(wrapped,select_physical(wrapped)));
 }
 auto a=scan({"a"},{{{"a",1}}}),b=scan({"b"},{{{"b",2}}});
 auto unequal=join(a,b,"a","b",Kind::Join,Compare::Less);auto wrong=std::make_shared<Plan>(*unequal);wrong->compare=Compare::Equal;wrong->kind=Kind::HashJoin;require(!equivalent(unequal,wrong));
 wrong->compare=Compare::Less;bool rejected=false;try{execute(wrong);}catch(const std::invalid_argument&){rejected=true;}require(rejected);
 auto payloads=scan({"b","v"},{{{"b",1},{"v",10}},{{"b",1},{"v",20}},{{"b",1},{"v",30}},{{"b",1},{"v",40}}});
 auto limited=unary(Kind::Limit,join(a,payloads,"a","b"));limited->count=2;
 auto selected=select_physical(limited);
 require(selected->left->kind==Kind::Join&&selected->left==limited->left);
 require(equivalent(limited,selected)&&execute(limited)==execute(selected));
 require(execute(selected)==Rows({{{"a",1},{"b",1},{"v",10}},{{"a",1},{"b",1},{"v",20}}}));
 auto wrappedLimit=filter(limited,{"v",0});auto selectedWrapper=select_physical(wrappedLimit);
 require(selectedWrapper->left->left->kind==Kind::Join&&equivalent(wrappedLimit,selectedWrapper));
 auto empty=join(scan({"a"},{}),scan({"b"},{}),"a","b");require(equivalent(empty,select_physical(empty)));
}

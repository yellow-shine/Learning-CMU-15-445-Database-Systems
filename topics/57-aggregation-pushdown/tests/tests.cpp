#include "aggregate.hpp"
int main(){using namespace db;
 for(int copies=0;copies<5;++copies){std::vector<int> keys(copies,1);keys.push_back(2);
 auto p=node(Kind::Aggregate,node(Kind::Join,facts({{1,10},{1,20},{1,std::nullopt},{2,std::nullopt},{3,7}}),dimension(keys)));
 auto q=push_aggregate(p);require(q->kind==Kind::Final&&equivalent(p,q));auto g=execute(q);
 if(copies)require(g.at(1).count==static_cast<std::size_t>(copies*2)&&g.at(1).sum==copies*30);
 require(g.at(2).count==0&&!g.at(2).average());}
 auto empty=node(Kind::Aggregate,node(Kind::Join,facts({}),dimension({1})));require(equivalent(empty,push_aggregate(empty))&&execute(empty).empty());
 auto outer=node(Kind::Aggregate,node(Kind::LeftJoin,facts({{3,7}}),dimension({1})));
 require(push_aggregate(outer)==outer);
 auto wrong=node(Kind::Final,node(Kind::StateJoin,node(Kind::Partial,outer->left->left),outer->left->right));require(!equivalent(outer,wrong));
 State a,b;a.add(0);a.add(10);b.add(100);State merged=a;merged.merge(b);
 require(merged.average()==110.0/3&&(*a.average()+*b.average())/2!=*merged.average());
 // Removing duplicate keys is not a legal optimization of SUM/COUNT.
 auto f=facts({{1,10}});auto p=node(Kind::Aggregate,node(Kind::Join,f,dimension({1,1})));
 auto dedup=node(Kind::Aggregate,node(Kind::Join,f,dimension({1})));require(!equivalent(p,dedup));
}

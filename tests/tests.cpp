#include "ordering.hpp"
int main(){using namespace db;
 for(int seed=0;seed<12;++seed)for(int n=1;n<=5;++n){Query q;for(int i=0;i<n;++i){std::vector<int> table;for(int j=0;j<(seed+i)%4;++j)table.push_back((j+seed)%3);q.tables.push_back(table);}
 auto dp=dynamic_program(q);auto all=exhaustive(q);auto expected=result(q,left_deep(q));long double best=all.front()->cost;
 for(auto p:all){best=std::min(best,p->cost);require(result(q,p)==expected);}require(dp->cost==best&&result(q,dp)==expected);}
 Query dup{{{1,1},{1,1,1},{1,1}}};require(result(dup,dynamic_program(dup)).size()==12);
 Query demo{{{0,1,0,1,0,1,0,1},{0,1},{0}}};require(dynamic_program(demo)->cost==10&&left_deep(demo)->cost==24);
 for(auto s:{Semantics::LeftJoin,Semantics::NonEquijoin}){bool bad=false;try{dynamic_program(Query{{{1},{2}},s});}catch(const std::invalid_argument&){bad=true;}require(bad);}
 bool bad=false;try{dynamic_program(Query{});}catch(const std::invalid_argument&){bad=true;}require(bad);
 bad=false;try{dynamic_program(Query{std::vector<std::vector<int>>(13)});}catch(const std::invalid_argument&){bad=true;}require(bad);
 // Empty and disjoint data do not invent rows even when estimates assume common domains.
 Query disjoint{{{1},{2},{3}}};require(result(disjoint,dynamic_program(disjoint)).empty()&&dynamic_program(disjoint)->rows==1);
 auto cards=cardinalities(dup);bad=false;try{combine(leaf(0,cards),leaf(0,cards),cards);}catch(const std::invalid_argument&){bad=true;}require(bad);
}

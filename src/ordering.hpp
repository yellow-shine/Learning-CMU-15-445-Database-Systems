#pragma once
#include <algorithm>
#include <cmath>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
namespace db {
using Mask=unsigned;
enum class Semantics {InnerEquijoin,LeftJoin,NonEquijoin};
// Every table joins on the same non-null integer key equivalence class.
struct Query {std::vector<std::vector<int>> tables;Semantics semantics=Semantics::InnerEquijoin;};
inline void validate(const Query&q){if(q.semantics!=Semantics::InnerEquijoin)throw std::invalid_argument("only inner equality supported");if(q.tables.empty()||q.tables.size()>12)throw std::invalid_argument("1..12 tables required");}
struct Plan;
using P=std::shared_ptr<Plan>;
struct Plan {Mask mask;int table=-1;P left,right;long double rows=0,cost=0;};
inline std::vector<long double> cardinalities(const Query&q){
 validate(q);std::vector<std::size_t> ndv;
 for(auto&t:q.tables)ndv.push_back(std::set<int>(t.begin(),t.end()).size());
 std::vector<long double> cards(1U<<q.tables.size());
 for(Mask mask=1;mask<cards.size();++mask){long double product=1;std::size_t largest=0,count=0;
  for(std::size_t i=0;i<q.tables.size();++i)if(mask&(1U<<i)){product*=q.tables[i].size();largest=std::max(largest,ndv[i]);++count;}
  cards[mask]=largest?product/std::pow(static_cast<long double>(largest),static_cast<int>(count-1)):0;
 }
 return cards;
}
inline P leaf(int i,const std::vector<long double>&cards){Mask m=1U<<i;return std::make_shared<Plan>(Plan{m,i,nullptr,nullptr,cards[m],0});}
inline P combine(P a,P b,const std::vector<long double>&cards){
 if(a->mask&b->mask)throw std::invalid_argument("overlapping join inputs");
 Mask m=a->mask|b->mask;return std::make_shared<Plan>(Plan{m,-1,a,b,cards.at(m),a->cost+b->cost+a->rows*b->rows});
}
inline P dynamic_program(const Query&q){
 auto cards=cardinalities(q);std::vector<P> best(cards.size());
 for(std::size_t i=0;i<q.tables.size();++i)best[1U<<i]=leaf(static_cast<int>(i),cards);
 for(Mask mask=1;mask<cards.size();++mask){if(best[mask])continue;
  Mask anchor=mask&(~mask+1U);
  // One representative per symmetric partition; costs do not distinguish build sides.
  for(Mask a=(mask-1)&mask;a;a=(a-1)&mask){Mask b=mask^a;if(!(a&anchor)||!b)continue;
   auto candidate=combine(best[a],best[b],cards);if(!best[mask]||candidate->cost<best[mask]->cost)best[mask]=candidate;}
 }
 return best.back();
}
inline std::vector<P> enumerate(Mask mask,const std::vector<long double>&cards){
 if((mask&(mask-1))==0){int i=0;while((1U<<i)!=mask)++i;return {leaf(i,cards)};}
 std::vector<P> out;Mask anchor=mask&(~mask+1U);
 for(Mask a=(mask-1)&mask;a;a=(a-1)&mask){Mask b=mask^a;if(!(a&anchor)||!b)continue;
  for(auto l:enumerate(a,cards))for(auto r:enumerate(b,cards))out.push_back(combine(l,r,cards));}
 return out;
}
inline std::vector<P> exhaustive(const Query&q){validate(q);if(q.tables.size()>6)throw std::invalid_argument("exhaustive limited to 6 tables");auto cards=cardinalities(q);return enumerate((1U<<q.tables.size())-1,cards);}
inline P left_deep(const Query&q){auto cards=cardinalities(q);P p=leaf(0,cards);for(std::size_t i=1;i<q.tables.size();++i)p=combine(p,leaf(static_cast<int>(i),cards),cards);return p;}
struct Row {int key;std::vector<int> ids;};
inline std::vector<Row> execute(const Query&q,P p,std::size_t&comparisons){
 if(p->table>=0){std::vector<Row> out;auto&t=q.tables.at(static_cast<std::size_t>(p->table));for(std::size_t i=0;i<t.size();++i){std::vector<int> ids(q.tables.size(),-1);ids.at(p->table)=static_cast<int>(i);out.push_back({t[i],ids});}return out;}
 auto left=execute(q,p->left,comparisons),right=execute(q,p->right,comparisons);std::vector<Row> out;
 for(auto&a:left)for(auto&b:right){++comparisons;if(a.key==b.key){Row r=a;for(std::size_t i=0;i<r.ids.size();++i)if(b.ids[i]>=0)r.ids[i]=b.ids[i];out.push_back(std::move(r));}}
 return out;
}
inline std::vector<std::vector<int>> result(const Query&q,P p){std::size_t count=0;std::vector<std::vector<int>> out;for(auto&r:execute(q,p,count))out.push_back(r.ids);std::sort(out.begin(),out.end());return out;}
inline std::string describe(P p){return p->table>=0?"T"+std::to_string(p->table):"("+describe(p->left)+" join "+describe(p->right)+")";}
inline void require(bool ok){if(!ok)throw std::runtime_error("check failed");}
}

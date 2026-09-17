#pragma once
#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>
namespace db {
using Cell=std::optional<int>;
using Row=std::map<std::string,Cell>;
using Rows=std::vector<Row>;
using Columns=std::set<std::string>;
enum class Kind {Scan,Filter,Project,Join,LeftJoin,Limit,Sort,HashJoin};
enum class Compare {Equal,Less,Greater};
struct Predicate {
 std::string column; int constant=0;
 bool matches(const Row& r) const {auto v=r.at(column); return v && *v>constant;}
 Columns references() const {return {column};}
};
struct Plan;
using P=std::shared_ptr<Plan>;
struct Plan {
 Kind kind=Kind::Scan; P left,right; Rows data; Columns columns;
 Predicate predicate; std::string leftKey,rightKey; Compare compare=Compare::Equal;
 std::size_t count=0;
};
inline bool subset(const Columns&a,const Columns&b) {return std::includes(b.begin(),b.end(),a.begin(),a.end());}
inline Columns schema(const P&p) {
 if(p->kind==Kind::Scan || p->kind==Kind::Project) return p->columns;
 auto s=schema(p->left);
 if(p->right) {auto r=schema(p->right); s.insert(r.begin(),r.end());}
 return s;
}
inline P scan(Columns columns, Rows rows) {
 for(const auto&r:rows) {Columns got; for(const auto&v:r)got.insert(v.first); if(got!=columns)throw std::invalid_argument("scan schema");}
 auto p=std::make_shared<Plan>();p->columns=std::move(columns);p->data=std::move(rows);return p;
}
inline P unary(Kind k,P child) {auto p=std::make_shared<Plan>();p->kind=k;p->left=std::move(child);return p;}
inline P filter(P child,Predicate pred) {if(!subset(pred.references(),schema(child)))throw std::invalid_argument("predicate column");auto p=unary(Kind::Filter,child);p->predicate=std::move(pred);return p;}
inline P project(P child,Columns cols) {if(!subset(cols,schema(child)))throw std::invalid_argument("projection column");auto p=unary(Kind::Project,child);p->columns=std::move(cols);return p;}
inline P join(P a,P b,std::string ak,std::string bk,Kind kind=Kind::Join,Compare cmp=Compare::Equal) {
 auto as=schema(a),bs=schema(b); for(auto&c:as)if(bs.count(c))throw std::invalid_argument("qualify columns");
 if(!as.count(ak)||!bs.count(bk))throw std::invalid_argument("join key");
 auto p=unary(kind,a);p->right=b;p->leftKey=ak;p->rightKey=bk;p->compare=cmp;return p;
}
inline bool match(Cell a,Cell b,Compare cmp) {if(!a||!b)return false;return cmp==Compare::Equal?*a==*b:cmp==Compare::Less?*a<*b:*a>*b;}
inline Row merge(Row a,const Row&b) {a.insert(b.begin(),b.end());return a;}
inline Rows execute(const P&p) {
 if(p->kind==Kind::Scan)return p->data;
 Rows a=execute(p->left),out;
 if(p->kind==Kind::Filter) {for(auto&r:a)if(p->predicate.matches(r))out.push_back(r);return out;}
 if(p->kind==Kind::Project) {for(auto&r:a){Row q;for(auto&c:p->columns)q[c]=r.at(c);out.push_back(q);}return out;}
 if(p->kind==Kind::Limit) {if(a.size()>p->count)a.resize(p->count);return a;}
 if(p->kind==Kind::Sort) {std::stable_sort(a.begin(),a.end(),[&](const Row&x,const Row&y){return x.at(p->leftKey)<y.at(p->leftKey);});return a;}
 auto b=execute(p->right);
 if(p->kind==Kind::HashJoin) {
  if(p->compare!=Compare::Equal)throw std::invalid_argument("hash requires equality");
  std::unordered_multimap<int,Row> index;
  for(auto&r:b)if(r.at(p->rightKey))index.emplace(*r.at(p->rightKey),r);
  for(auto&r:a)if(r.at(p->leftKey)){auto range=index.equal_range(*r.at(p->leftKey));for(auto it=range.first;it!=range.second;++it)out.push_back(merge(r,it->second));}
  return out;
 }
 for(auto&r:a) {bool found=false;for(auto&s:b)if(match(r.at(p->leftKey),s.at(p->rightKey),p->compare)){out.push_back(merge(r,s));found=true;}
  if(!found&&p->kind==Kind::LeftJoin){Row nulls;for(auto&c:schema(p->right))nulls[c]=std::nullopt;out.push_back(merge(r,nulls));}}
 return out;
}
inline bool equivalent(P a,P b) {auto x=execute(a),y=execute(b);std::sort(x.begin(),x.end());std::sort(y.begin(),y.end());return x==y;}
inline void require(bool value) {if(!value)throw std::runtime_error("check failed");}
}

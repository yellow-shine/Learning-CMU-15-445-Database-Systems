#pragma once
#include "plan.hpp"
#include <iterator>
namespace db {
inline Columns intersect(const Columns&a,const Columns&b){Columns c;std::set_intersection(a.begin(),a.end(),b.begin(),b.end(),std::inserter(c,c.end()));return c;}
inline P prune(P p,Columns needed) {
 if(!subset(needed,schema(p)))throw std::invalid_argument("required column missing");
 auto q=std::make_shared<Plan>(*p);
 switch(p->kind){
 case Kind::Scan:return project(p,needed);
 case Kind::Project:q->left=prune(p->left,needed);q->columns=needed;return q;
 case Kind::Filter:{auto input=needed;input.insert(p->predicate.column);q->left=prune(p->left,input);break;}
 case Kind::Aggregate:q->left=prune(p->left,{p->leftKey,p->rightKey});break;
 case Kind::Join:case Kind::LeftJoin:case Kind::HashJoin:{
  auto l=intersect(needed,schema(p->left)),r=intersect(needed,schema(p->right));
  l.insert(p->leftKey);r.insert(p->rightKey);q->left=prune(p->left,l);q->right=prune(p->right,r);break;}
 case Kind::Sort:{auto input=needed;input.insert(p->leftKey);q->left=prune(p->left,input);break;}
 case Kind::Limit:q->left=prune(p->left,needed);break;
 }
 return project(q,needed);
}
}

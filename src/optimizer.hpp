#pragma once
#include "plan.hpp"
namespace db {
inline P limit(P p,std::size_t count){auto q=unary(Kind::Limit,p);q->count=count;return q;}
inline P push_limit(P p){
 if(p->kind!=Kind::Limit)return p;
 auto child=p->left;
 if(child->kind==Kind::Project){auto q=std::make_shared<Plan>(*child);q->left=push_limit(limit(child->left,p->count));return q;}
 if(child->kind==Kind::Limit)return push_limit(limit(child->left,std::min(p->count,child->count)));
 // Filter, Sort and both joins can change which rows belong to the prefix.
 return p;
}
}

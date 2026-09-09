#pragma once
#include "plan.hpp"
namespace db {
inline P select_physical(P p){
 auto q=std::make_shared<Plan>(*p);
 if(p->left)q->left=select_physical(p->left);
 if(p->right)q->right=select_physical(p->right);
 // Hash buckets implement equality only. Left outer join is not supported by this hash executor.
 if(p->kind==Kind::Join&&p->compare==Compare::Equal)q->kind=Kind::HashJoin;
 return q;
}
inline const char* implementation(P p){return p->kind==Kind::HashJoin?"HashJoin":"NestedLoop";}
}

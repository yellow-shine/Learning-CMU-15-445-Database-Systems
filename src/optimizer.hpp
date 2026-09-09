#pragma once
#include "plan.hpp"
namespace db {
inline P push_predicate(P root) {
 if(root->kind!=Kind::Filter)return root;
 auto child=root->left;
 // Conservative: outer joins remain a boundary even for null-rejecting predicates.
 if(child->kind!=Kind::Join)return root;
 auto refs=root->predicate.references();
 auto replacement=std::make_shared<Plan>(*child);
 if(subset(refs,schema(child->left)))replacement->left=filter(child->left,root->predicate);
 else if(subset(refs,schema(child->right)))replacement->right=filter(child->right,root->predicate);
 else return root;
 return replacement;
}
}

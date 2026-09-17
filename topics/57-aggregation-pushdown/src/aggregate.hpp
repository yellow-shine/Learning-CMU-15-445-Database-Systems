#pragma once
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>
namespace db {
struct Fact {int key;std::optional<double> value;};
struct State {double sum=0;std::size_t count=0;
 void add(std::optional<double> v){if(v){sum+=*v;++count;}}
 void merge(const State&s){sum+=s.sum;count+=s.count;}
 std::optional<double> average()const {return count?std::optional<double>(sum/count):std::nullopt;}
};
using Groups=std::map<int,State>;
enum class Kind {Facts,Dimension,Join,LeftJoin,Aggregate,Partial,StateJoin,Final};
struct Plan;
using P=std::shared_ptr<Plan>;
struct Plan {Kind kind;P left,right;std::vector<Fact> facts;std::vector<int> keys;};
inline P node(Kind kind,P a=nullptr,P b=nullptr){return std::make_shared<Plan>(Plan{kind,a,b,{},{}});}
inline P facts(std::vector<Fact> rows){auto p=node(Kind::Facts);p->facts=std::move(rows);return p;}
inline P dimension(std::vector<int> keys){auto p=node(Kind::Dimension);p->keys=std::move(keys);return p;}
inline std::vector<Fact> rows(P p) {
 if(p->kind==Kind::Facts)return p->facts;
 if((p->kind!=Kind::Join&&p->kind!=Kind::LeftJoin)||p->right->kind!=Kind::Dimension)throw std::invalid_argument("row plan");
 std::vector<Fact> out;
 for(auto f:rows(p->left)){bool found=false;for(int key:p->right->keys)if(f.key==key){out.push_back(f);found=true;}
 if(!found&&p->kind==Kind::LeftJoin)out.push_back(f);}
 return out;
}
// StateJoin deliberately returns a bag: duplicate dimension keys replicate states.
inline std::vector<std::pair<int,State>> states(P p){
 if(p->kind==Kind::Partial){Groups g;for(auto f:rows(p->left))g[f.key].add(f.value);return {g.begin(),g.end()};}
 if(p->kind!=Kind::StateJoin||p->right->kind!=Kind::Dimension)throw std::invalid_argument("state plan");
 std::vector<std::pair<int,State>> out;
 for(auto s:states(p->left))for(int k:p->right->keys)if(k==s.first)out.push_back(s);
 return out;
}
inline Groups execute(P p){
 Groups out;
 if(p->kind==Kind::Aggregate){for(auto f:rows(p->left))out[f.key].add(f.value);}
 else if(p->kind==Kind::Final){for(auto s:states(p->left))out[s.first].merge(s.second);}
 else throw std::invalid_argument("expected aggregate root");
 return out;
}
inline P push_aggregate(P p){
 // Only grouping by the fact join key and aggregating fact values is represented.
 if(p->kind!=Kind::Aggregate||p->left->kind!=Kind::Join)return p;
 auto j=p->left;
 if(j->left->kind!=Kind::Facts||j->right->kind!=Kind::Dimension)return p;
 return node(Kind::Final,node(Kind::StateJoin,node(Kind::Partial,j->left),j->right));
}
inline bool equivalent(P a,P b){auto x=execute(a),y=execute(b);if(x.size()!=y.size())return false;
 auto i=x.begin(),j=y.begin();for(;i!=x.end();++i,++j)if(i->first!=j->first||i->second.sum!=j->second.sum||i->second.count!=j->second.count)return false;return true;}
inline void require(bool ok){if(!ok)throw std::runtime_error("check failed");}
}

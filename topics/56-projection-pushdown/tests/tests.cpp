#include "optimizer.hpp"
int main(){using namespace db;
 auto a=scan({"a","v","junk"},{{{"a",1},{"v",10},{"junk",0}},{{"a",1},{"v",20},{"junk",0}}});
 auto b=scan({"b","w"},{{{"b",1},{"w",8}},{{"b",1},{"w",9}}});
 auto p=project(filter(join(a,b,"a","b"),{"w",8}),{"v"});auto q=prune(p,{"v"});require(equivalent(p,q)&&execute(q).size()==2);
 auto g=unary(Kind::Aggregate,join(a,b,"a","b"));g->leftKey="a";g->rightKey="v";
 auto output=project(g,{"sum"});require(equivalent(output,prune(output,{"sum"})));require(*execute(prune(output,{"sum"}))[0].at("sum")==60);
 auto grouped=prune(g,{"a"});require(execute(grouped).size()==1); // grouping key must survive even without sum output
 auto empty=project(scan({"x"},{}),{});require(equivalent(empty,prune(empty,{})));
 require(execute(prune(a,{})).size()==2); // zero columns still retain duplicate rows
 bool rejected=false;try{prune(a,{"unknown"});}catch(const std::invalid_argument&){rejected=true;}require(rejected);
 rejected=false;try{filter(project(a,{"a"}),{"v",1});}catch(const std::invalid_argument&){rejected=true;}require(rejected); // unsafe user-only pruning
 auto sorted=unary(Kind::Sort,a);sorted->leftKey="v";require(equivalent(project(sorted,{"a"}),prune(sorted,{"a"})));
}

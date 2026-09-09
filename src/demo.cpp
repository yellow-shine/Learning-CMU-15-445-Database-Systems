#include "cost.hpp"
#include <iostream>
int main(){using namespace db;std::vector<int>a{1,2,3,4},b{1,2,3,4};
 for(auto algorithm:{Algorithm::NestedLoop,Algorithm::HashJoin}){
 auto predicted=estimate(algorithm,a.size(),b.size(),2,4);auto r=execute({algorithm,2,predicted},a,b);
 std::cout<<(algorithm==Algorithm::HashJoin?"hash":"nested")<<" pages="<<r.measured.pages<<" ops="<<r.measured.operations<<" output="<<r.measured.outputs<<" estimated cost="<<cost(predicted)<<" measured cost="<<cost(r.measured)<<'\n';}
 require(choose(4,4,2,4).algorithm==Algorithm::HashJoin);
}

#include "engine.h"
#include <iostream>
int main() {

VectorizedQuery query({{1,5},{2,20},{3,10},{2,20}},2,10); query.Init();
std::vector<int> keys;
while(query.Next(keys)) { std::cout << "batch:"; for(auto key:keys) std::cout << ' ' << key; std::cout << '\n'; }
std::cout << "scan calls=" << query.scan().calls << " reads=" << query.scan().reads
          << " filter calls=" << query.filter_calls << " projection calls=" << query.projection_calls << '\n';

}

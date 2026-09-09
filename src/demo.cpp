#include "engine.h"
#include <iostream>
int main() {

PipelineQuery query({{1,30},{2,5},{3,10},{4,10}},10); query.Init();
int key; std::vector<int> result; while(query.Next(key)) result.push_back(key);
for(const auto& event:query.trace) std::cout << event << '\n';
std::cout << "result:"; for(auto x:result) std::cout << ' ' << x; std::cout << '\n';

}

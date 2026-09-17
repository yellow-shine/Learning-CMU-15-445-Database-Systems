#include "engine.h"
#include <iostream>
#include <stdexcept>
#define CHECK(...) do { if (!(__VA_ARGS__)) throw std::runtime_error("check failed: " #__VA_ARGS__); } while(false)
int main() { try {

std::vector<Row> rows{{1,5},{2,20},{3,10},{2,20},{9,-1}};
for (std::size_t width : {1u,2u,3u,99u}) {
    VectorizedQuery query(rows,width,10); query.Init();
    std::vector<int> keys, result;
    while(query.Next(keys)) { CHECK(!keys.empty() && keys.size()<=width); result.insert(result.end(),keys.begin(),keys.end()); }
    CHECK(result==std::vector<int>({2,3,2})); CHECK(query.scan().reads==5);
    CHECK(query.scan().calls==(5+width-1)/width+1);
    auto calls=query.scan().calls; CHECK(!query.Next(keys) && keys.empty() && query.scan().calls==calls);
    query.Init(); CHECK(query.Next(keys));
}
VectorizedQuery gaps({{1,0},{2,0},{3,10}},2,10); gaps.Init(); std::vector<int> keys;
CHECK(gaps.Next(keys) && keys==std::vector<int>({3}));
VectorizedQuery empty({},2,10); empty.Init(); CHECK(!empty.Next(keys) && !empty.Next(keys));
VectorizedQuery rejected({{1,0}},2,10); rejected.Init(); CHECK(!rejected.Next(keys));
bool threw=false; try { VectorizedQuery bad({},0,0); } catch(const std::invalid_argument&) { threw=true; } CHECK(threw);
Batch batch{{{1,0},{2,20},{3,10}}, {0,1,2}}; Filter(batch,10);
CHECK(batch.selection==std::vector<std::size_t>({1,2}) && batch.rows.size()==3);
CHECK(Projection(batch)==std::vector<int>({2,3}));

std::cout << "all checks passed\n";
} catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

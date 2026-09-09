#include "engine.h"
#include <iostream>
#include <stdexcept>
#define CHECK(...) do { if (!(__VA_ARGS__)) throw std::runtime_error("check failed: " #__VA_ARGS__); } while(false)
int main() { try {

MaterializedQuery query({{1,5},{2,20},{3,10},{2,20}},10);
query.Init(); CHECK(query.stats.reads==4 && query.result_size()==3);
CHECK(query.stats.scan_calls==1 && query.stats.filter_calls==1 && query.stats.projection_calls==1);
CHECK(query.intermediate_bytes()==7*sizeof(Row)+3*sizeof(int));
int key=-1; std::vector<int> result; while(query.Next(key)) result.push_back(key);
CHECK(result==std::vector<int>({2,3,2})); CHECK(!query.Next(key) && key==2 && query.stats.reads==4);
query.Init(); CHECK(query.Next(key) && key==2 && query.stats.scan_calls==1);
MaterializedQuery empty({},10); empty.Init(); CHECK(!empty.Next(key) && !empty.Next(key)); CHECK(empty.intermediate_bytes()==0);
MaterializedQuery rejected({{1,1}},10); rejected.Init(); CHECK(!rejected.Next(key)); CHECK(rejected.stats.reads==1);

std::cout << "all checks passed\n";
} catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

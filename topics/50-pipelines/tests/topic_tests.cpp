#include "engine.h"
#include <iostream>
#include <stdexcept>
#define CHECK(...) do { if (!(__VA_ARGS__)) throw std::runtime_error("check failed: " #__VA_ARGS__); } while(false)
int main() { try {

PipelineQuery query({{1,30},{2,5},{3,10},{4,10}},10); query.Init(); CHECK(query.reads==0 && query.trace.empty());
int key=-1; CHECK(query.Next(key) && key==3); CHECK(query.reads==4 && query.buffered_rows()==3);
const std::vector<std::string> prefix{"P1.begin","scan:1","filter.pass:1","sort.sink:1","scan:2","filter.drop:2","scan:3","filter.pass:3","sort.sink:3","scan:4","filter.pass:4","sort.sink:4","P1.end","sort.ready","P2.begin","sort.source:3","project:3"};
CHECK(query.trace==prefix);
CHECK(query.Next(key) && key==4); CHECK(query.Next(key) && key==1); CHECK(!query.Next(key));
auto count=query.trace.size(); CHECK(!query.Next(key) && query.trace.size()==count && key==1);
query.Init(); CHECK(query.Next(key) && key==3 && query.reads==4);
PipelineQuery empty({},0); empty.Init(); CHECK(!empty.Next(key));
CHECK(empty.trace==std::vector<std::string>({"P1.begin","P1.end","sort.ready","P2.begin","P2.end"}));
PipelineQuery rejected({{1,-1}},0); rejected.Init(); CHECK(!rejected.Next(key) && rejected.reads==1 && rejected.buffered_rows()==0);

std::cout << "all checks passed\n";
} catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

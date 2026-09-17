#include "engine.h"
#include <iostream>
#include <stdexcept>
#define CHECK(...) do { if (!(__VA_ARGS__)) throw std::runtime_error("check failed: " #__VA_ARGS__); } while(false)
int main() { try {


Scan scan({0,1,2,3,4,5}); Limit query(scan,2,2); query.Init(); int value=-1;
CHECK(query.Next(value) && value==2 && scan.reads==3);
CHECK(query.Next(value) && value==3 && scan.reads==4);
CHECK(!query.Next(value) && !query.Next(value) && value==3 && scan.calls==4);
query.Init(); CHECK(query.Next(value) && value==2 && scan.reads==3);
for(std::size_t n=0;n<8;++n) for(std::size_t offset=0;offset<10;++offset) for(std::size_t limit=0;limit<10;++limit) {
    std::vector<int> input; for(std::size_t i=0;i<n;++i) input.push_back(static_cast<int>(i));
    Scan source(input); Limit op(source,limit,offset); op.Init(); std::vector<int> output;
    while(op.Next(value)) output.push_back(value);
    std::vector<int> expected;
    for(std::size_t i=offset;i<n && expected.size()<limit;++i) expected.push_back(input[i]);
    CHECK(output==expected);
    const auto expected_reads=limit==0?0:std::min(n,offset+limit);
    CHECK(source.reads==expected_reads);
    CHECK(source.calls==expected_reads+((limit>0 && (offset>n || limit>n-offset))?1:0));
    auto calls=source.calls; CHECK(!op.Next(value) && source.calls==calls);
}
auto huge=std::numeric_limits<std::size_t>::max();
Scan small({7,8}); Limit far(small,huge,huge); far.Init(); CHECK(!far.Next(value) && small.reads==2 && small.calls==3);
Limit zero(small,0,huge); zero.Init(); CHECK(!zero.Next(value) && small.calls==0);
Limit all(small,huge,0); all.Init(); CHECK(all.Next(value) && value==7); CHECK(all.Next(value) && value==8); CHECK(!all.Next(value));
Scan nested_scan({0,1,2,3,4,5}); Limit inner(nested_scan,4,1); Limit outer(inner,1,1);
outer.Init(); CHECK(outer.Next(value) && value==2); CHECK(!outer.Next(value) && nested_scan.reads==3);

std::cout << "all checks passed\n";
} catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

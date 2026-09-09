#include "engine.h"
#include <iostream>
#include <stdexcept>
#define CHECK(...) do { if (!(__VA_ARGS__)) throw std::runtime_error("check failed: " #__VA_ARGS__); } while(false)
int main() { try {

AccessTable table({{8,80},{2,20},{5,1},{2,25},{9,90},{4,40}});
auto seq=table.SeqScan(2,4,22), idx=table.IndexScan(2,4,22);
CHECK(seq.rids==std::vector<std::size_t>({3,5})); CHECK(idx.rids==seq.rids);
CHECK(seq.heap_reads==6 && idx.heap_reads==3 && idx.index_entries==3 && idx.seek_comparisons>0);
std::vector<Row> rows;
for(int i=0;i<101;++i) rows.push_back({(i*17)%13-6,i%9-4});
AccessTable many(rows);
for(int low=-8;low<=8;++low) for(int high=-8;high<=8;++high) for(int minimum : {-5,0,5}) {
    auto a=many.SeqScan(low,high,minimum), b=many.IndexScan(low,high,minimum);
    std::sort(b.rids.begin(),b.rids.end()); CHECK(a.rids==b.rids);
    CHECK(b.heap_reads<=a.heap_reads);
    for(auto rid:a.rids) CHECK(many.row(rid).key>=low && many.row(rid).key<=high);
}
AccessTable empty({}); CHECK(empty.SeqScan(0,9,0).rids.empty()); CHECK(empty.IndexScan(0,9,0).heap_reads==0);
CHECK(table.IndexScan(20,30,0).rids.empty()); CHECK(table.IndexScan(9,2,0).heap_reads==0);
CHECK(table.IndexScan(-2147483647-1,2147483647,-2147483647-1).rids.size()==6);

std::cout << "all checks passed\n";
} catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

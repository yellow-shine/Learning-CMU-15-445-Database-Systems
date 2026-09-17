#include "engine.h"
#include <iostream>
int main() {

AccessTable table({{8,80},{2,20},{5,1},{2,25},{9,90},{4,40}});
auto seq=table.SeqScan(2,4,22), idx=table.IndexScan(2,4,22);
std::cout << "SeqScan RIDs:"; for(auto rid:seq.rids) std::cout << ' ' << rid;
std::cout << " heap reads=" << seq.heap_reads << '\n';
std::cout << "IndexScan RIDs:"; for(auto rid:idx.rids) std::cout << ' ' << rid;
std::cout << " heap reads=" << idx.heap_reads << " index entries=" << idx.index_entries
          << " seek comparisons=" << idx.seek_comparisons << '\n';

}

#include "engine.h"
#include <iostream>
int main() {

MaterializedQuery query({{1,5},{2,20},{3,10},{2,20}},10); query.Init();
int key; while(query.Next(key)) std::cout << key << ' ';
std::cout << "\noperator calls=" << query.stats.scan_calls << ',' << query.stats.filter_calls << ','
          << query.stats.projection_calls << " reads=" << query.stats.reads
          << " intermediate bytes=" << query.intermediate_bytes() << '\n';

}

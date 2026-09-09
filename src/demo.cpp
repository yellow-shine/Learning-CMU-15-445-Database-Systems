#include "engine.h"
#include <iostream>
int main() {

const Frame rows{FrameUnit::Rows,Bound::UnboundedPreceding,Bound::CurrentRow};
auto result=Window({{2,2,7},{1,20,5},{1,10,3},{1,20,-2},{2,1,4},{1,30,1}},rows);
std::cout << "partition order amount row_number rank dense_rank sum\n";
for(const auto& r:result) std::cout << r.row.partition << ' ' << r.row.order << ' ' << r.row.amount << ' '
    << r.row_number << ' ' << r.rank << ' ' << r.dense_rank << ' ' << r.sum << '\n';

}

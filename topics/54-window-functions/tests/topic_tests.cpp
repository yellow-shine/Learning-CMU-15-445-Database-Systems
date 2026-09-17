#include "engine.h"
#include <iostream>
#include <stdexcept>
#define CHECK(...) do { if (!(__VA_ARGS__)) throw std::runtime_error("check failed: " #__VA_ARGS__); } while(false)
int main() { try {

const Frame rows{FrameUnit::Rows,Bound::UnboundedPreceding,Bound::CurrentRow};
auto result=Window({{2,2,7},{1,20,5},{1,10,3},{1,20,-2},{2,1,4},{1,30,1}},rows);
CHECK(result.size()==6);
CHECK(result[0].input_position==2 && result[0].sum==3 && result[0].row_number==1);
CHECK(result[1].rank==2 && result[1].dense_rank==2 && result[1].sum==8);
CHECK(result[2].row_number==3 && result[2].rank==2 && result[2].dense_rank==2 && result[2].sum==6);
CHECK(result[3].rank==4 && result[3].dense_rank==3 && result[3].sum==7);
CHECK(result[4].rank==1 && result[4].sum==4 && result[5].sum==11);
CHECK(Window({},rows).empty()); auto single=Window({{0,0,-3}},rows); CHECK(single[0].sum==-3 && single[0].dense_rank==1);
for(auto frame : {Frame{FrameUnit::Range,Bound::UnboundedPreceding,Bound::CurrentRow},
                  Frame{FrameUnit::Groups,Bound::UnboundedPreceding,Bound::CurrentRow},
                  Frame{FrameUnit::Rows,Bound::CurrentRow,Bound::CurrentRow},
                  Frame{FrameUnit::Rows,Bound::UnboundedPreceding,Bound::UnboundedFollowing}}) {
    bool threw=false; try { Window({},frame); } catch(const std::invalid_argument&) { threw=true; } CHECK(threw);
}
for(auto amount : {std::numeric_limits<std::int64_t>::max(), std::numeric_limits<std::int64_t>::min()}) {
    bool threw=false; try { Window({{0,0,amount},{0,1,amount>0?1:-1}},rows); } catch(const std::overflow_error&) { threw=true; } CHECK(threw);
}
CHECK(Window({{0,0,std::numeric_limits<std::int64_t>::min()},{0,1,std::numeric_limits<std::int64_t>::max()}},rows)[1].sum==-1);
std::vector<Row> input;
for(int i=0;i<40;++i) input.push_back({i%3,(i*7)%5,i%9-4});
auto actual=Window(input,rows);
for(const auto& row:actual) {
    std::size_t number=0, rank=1; std::vector<int> distinct;
    std::int64_t sum=0;
    for(std::size_t j=0;j<input.size();++j) if(input[j].partition==row.row.partition) {
        if(input[j].order<row.row.order) { ++rank; distinct.push_back(input[j].order); }
        if(input[j].order<row.row.order || (input[j].order==row.row.order && j<=row.input_position)) { ++number; sum+=input[j].amount; }
    }
    std::sort(distinct.begin(),distinct.end()); distinct.erase(std::unique(distinct.begin(),distinct.end()),distinct.end());
    CHECK(row.row_number==number && row.rank==rank && row.dense_rank==distinct.size()+1 && row.sum==sum);
}

std::cout << "all checks passed\n";
} catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

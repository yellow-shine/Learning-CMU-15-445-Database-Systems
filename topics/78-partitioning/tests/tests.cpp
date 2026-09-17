#include "partitioning.hpp"
#include "check.hpp"
#include <limits>
#include <numeric>
int main() {
 using namespace tutorial;
 RangeRouter r({0,10});
 CHECK(r.route(-1)==0 && r.route(0)==1 && r.route(9)==1 && r.route(10)==2);
 CHECK(r.route(std::numeric_limits<std::int64_t>::min())==0);
 CHECK(r.route(std::numeric_limits<std::int64_t>::max())==2);
 CHECK(RangeRouter({}).route(99)==0);
 rejects([]{RangeRouter x({1,1});}); rejects([]{RangeRouter x({2,1});});
 rejects([]{hash_route(1,0);});
 CHECK(hash_route(std::numeric_limits<std::int64_t>::min(),7)<7);
 auto empty=statistics(std::vector<std::int64_t>{},3,[&](auto k){return r.route(k);});
 CHECK(empty.counts==std::vector<std::size_t>({0,0,0}) && empty.skew==0);
 auto s=statistics(std::vector<std::int64_t>{-1,0,9,10,10},3,[&](auto k){return r.route(k);});
 CHECK(s.counts==std::vector<std::size_t>({1,2,2})); CHECK(s.skew==1.2);
 rejects([]{statistics(std::vector<std::int64_t>{1},1,[](auto){return 2U;});});
 std::vector<std::int64_t> keys; for(int i=-1000;i<=1000;++i) keys.push_back(i);
 auto h=statistics(keys,7,[](auto k){return hash_route(k,7);});
 CHECK(std::accumulate(h.counts.begin(),h.counts.end(),std::size_t{0})==keys.size());
}

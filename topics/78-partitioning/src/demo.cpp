#include "partitioning.hpp"
#include <iostream>
int main() {
  tutorial::RangeRouter r({0,10});
  auto a=tutorial::statistics(std::vector<std::int64_t>{-1,0,9,10,10},r.partitions(),[&](auto k){return r.route(k);});
  auto b=tutorial::statistics(std::vector<std::int64_t>{0,4,8,12},4,[](auto k){return tutorial::hash_route(k,4);});
  std::cout<<"range:"; for(auto n:a.counts) std::cout<<' '<<n; std::cout<<" skew="<<a.skew<<'\n';
  std::cout<<"hash:"; for(auto n:b.counts) std::cout<<' '<<n; std::cout<<" skew="<<b.skew<<'\n';
}

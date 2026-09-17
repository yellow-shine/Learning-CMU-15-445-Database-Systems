#include "distributed_query.hpp"
#include "check.hpp"
#include <algorithm>
#include <random>
using namespace tutorial;
Joined oracle(const Rows& l,const Rows& r) {Joined out; for(auto a:l) for(auto b:r) if(a.key==b.key) out.emplace_back(a.key,a.value,b.value); std::sort(out.begin(),out.end()); return out;}
void compare(const Rows& l,const Rows& r,std::size_t p) {
 auto expected=oracle(l,r),b=broadcast_join(l,r,p).rows,h=repartition_join(l,r,p).rows;
 std::sort(b.begin(),b.end()); std::sort(h.begin(),h.end()); CHECK(b==expected && h==expected);
 CHECK(broadcast_join(l,r,p).sent==p*r.size()); CHECK(repartition_join(l,r,p).sent==l.size()+r.size());
 Shards shards(p); std::map<int,Aggregate> a;
 for(std::size_t i=0;i<l.size();++i) {auto row=l[i]; shards[i%p].push_back(row); a[row.key].sum+=row.value; ++a[row.key].count;}
 CHECK(partial_aggregate(shards,p).groups==a);
}
int main() {
 for(std::size_t p: {1,2,3,8}) {
  compare({}, {},p); compare({{1,2}}, {},p); compare({},{{1,2}},p);
  compare({{1,2},{1,2},{1,3}},{{1,4},{1,4}},p);
  std::mt19937 rng(445); for(int test=0;test<100;++test) {
   Rows l,r; for(int i=0;i<test%23;++i) l.push_back({static_cast<int>(rng()%7)-3,static_cast<int>(rng()%100)-50});
   for(int i=0;i<test%19;++i) r.push_back({static_cast<int>(rng()%7)-3,static_cast<int>(rng()%100)-50});
   compare(l,r,p);
  }
 }
 auto a=partial_aggregate({{{1,0}},{{1,30},{1,30},{1,60}}},2); CHECK(a.groups.at(1).average()==30); CHECK(a.sent==2);
 rejects([]{broadcast_join({}, {},0);}); rejects([]{repartition_join({}, {},0);}); rejects([]{partial_aggregate({},0);});
 rejects([]{Aggregate{}.average();});
 Aggregate full{std::numeric_limits<std::int64_t>::max(),1}; rejects([&]{merge(full,{1,1});}); CHECK(full.count==1);
 Aggregate low{std::numeric_limits<std::int64_t>::min(),1}; rejects([&]{merge(low,{-1,1});});
 Aggregate count{0,std::numeric_limits<std::uint64_t>::max()}; rejects([&]{merge(count,{0,1});});
}

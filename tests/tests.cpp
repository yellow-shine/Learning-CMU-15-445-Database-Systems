#include "statistics.hpp"
#include <climits>
int main(){using namespace db;
 auto e=analyze({},3);require(e.rows==0&&e.less(0)==0&&e.equal(0)==0&&equijoin(e,e)==0);
 auto nulls=analyze({std::nullopt,std::nullopt},2);require(nulls.nulls==2&&!nulls.minimum&&nulls.less(100)==0);
 auto s=analyze({0,1,2,3,4,5,6,7,8,9},5);require(s.distinct==10&&s.minimum==0&&s.maximum==9);
 for(int i=-2;i<12;++i){require(close(s.less(i),std::clamp(i/10.0L,0.0L,1.0L)));require(close(s.equal(i),i>=0&&i<10?.1L:0.0L));}
 auto constant=analyze({7,7,std::nullopt},99);require(close(constant.equal(7),2.0L/3)&&constant.less(7)==0&&close(constant.less(8),2.0L/3));
 auto extreme=analyze({INT_MIN,INT_MAX},2);require(extreme.less(INT_MIN)==0&&extreme.less(static_cast<long double>(INT_MAX)+1)==1);
 auto skew=analyze({0,0,0,9},1);require(close(skew.equal(0),.1L)&&!close(skew.equal(0),.75L));
 require(close(equijoin(s,s),10));require(equijoin(s,analyze({100},1))==0);
 bool bad=false;try{analyze({1},0);}catch(const std::invalid_argument&){bad=true;}require(bad);
 bad=false;try{conjunction(-1,.5);}catch(const std::invalid_argument&){bad=true;}require(bad);
 bad=false;try{s.less(std::numeric_limits<long double>::quiet_NaN());}catch(const std::invalid_argument&){bad=true;}require(bad);
 // Bucket totals and monotonic CDF, including fractional-width buckets.
 for(std::size_t bins=1;bins<12;++bins){auto h=analyze({0,0,1,3,9},bins);std::size_t count=0;for(auto b:h.buckets)count+=b.count;require(count==5);
 long double prev=0;for(int x=-1;x<=11;++x){auto v=h.less(x);require(v>=prev&&v<=1);prev=v;}require(prev==1);}
}

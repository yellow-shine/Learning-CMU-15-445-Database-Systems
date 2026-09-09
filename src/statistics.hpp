#pragma once
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>
namespace db {
struct Bucket {long double lo,hi;std::size_t count=0;};
struct Statistics {
 std::size_t rows=0,nulls=0,distinct=0;
 std::optional<int> minimum,maximum;
 std::vector<Bucket> buckets;
 long double less(long double x)const {
  if(!std::isfinite(x))throw std::invalid_argument("finite threshold required");
  if(!rows)return 0;
  long double count=0;
  for(auto b:buckets)count+=b.count*std::clamp((x-b.lo)/(b.hi-b.lo),0.0L,1.0L);
  return count/rows;
 }
 long double equal(int x)const {
  if(!minimum||x<*minimum||x>*maximum)return 0;
  // Integers occupy [x,x+1), potentially crossing fractional bucket edges.
  return less(static_cast<long double>(x)+1)-less(x);
 }
 long double nonnull()const {return static_cast<long double>(rows-nulls);}
};
inline Statistics analyze(const std::vector<std::optional<int>>& data,std::size_t requested){
 if(!requested)throw std::invalid_argument("positive bucket count required");
 Statistics s;s.rows=data.size();std::set<int> values;
 for(auto v:data)if(v)values.insert(*v);else ++s.nulls;
 s.distinct=values.size();if(values.empty())return s;
 s.minimum=*values.begin();s.maximum=*values.rbegin();
 long double lo=*s.minimum,span=static_cast<long double>(*s.maximum)-lo+1;
 auto n=std::min(requested,data.size()-s.nulls);
 for(std::size_t i=0;i<n;++i)s.buckets.push_back({lo+span*i/n,lo+span*(i+1)/n,0});
 for(auto v:data)if(v){auto i=static_cast<std::size_t>((static_cast<long double>(*v)-lo)/span*n);++s.buckets.at(std::min(i,n-1)).count;}
 return s;
}
inline long double conjunction(long double a,long double b){
 if(!std::isfinite(a)||!std::isfinite(b)||a<0||a>1||b<0||b>1)throw std::invalid_argument("selectivity in [0,1]");return a*b;
}
inline long double equijoin(const Statistics&a,const Statistics&b){
 if(!a.distinct||!b.distinct)return 0;
 if(*a.maximum<*b.minimum||*b.maximum<*a.minimum)return 0;
 return a.nonnull()*b.nonnull()/std::max(a.distinct,b.distinct);
}
inline void require(bool ok){if(!ok)throw std::runtime_error("check failed");}
inline bool close(long double a,long double b){return std::abs(a-b)<1e-10L;}
}

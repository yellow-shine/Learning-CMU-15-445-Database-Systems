#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>
namespace db {
enum class Algorithm {NestedLoop,HashJoin};
struct Counts {long double pages=0,operations=0,outputs=0;};
struct Weights {long double io=10,cpu=1;};
inline long double cost(Counts c,Weights w={}){
 if(!std::isfinite(w.io)||!std::isfinite(w.cpu)||w.io<0||w.cpu<0)throw std::invalid_argument("nonnegative finite weights");
 return c.pages*w.io+(c.operations+c.outputs)*w.cpu;
}
inline std::size_t pages(std::size_t n,std::size_t capacity){if(!capacity)throw std::invalid_argument("positive page capacity");return n/capacity+(n%capacity!=0);}
inline Counts estimate(Algorithm a,std::size_t n,std::size_t m,std::size_t capacity,long double output){
 if(!std::isfinite(output)||output<0||output>static_cast<long double>(n)*m)throw std::invalid_argument("invalid output estimate");
 long double left=pages(n,capacity),right=pages(m,capacity);
 return a==Algorithm::NestedLoop?Counts{left+n*right,static_cast<long double>(n)*m,output}:Counts{left+right,static_cast<long double>(n)+m,output};
}
struct Plan {Algorithm algorithm;std::size_t pageCapacity;Counts estimated;};
inline Plan choose(std::size_t n,std::size_t m,std::size_t capacity,long double output,Weights w={}){
 auto nested=estimate(Algorithm::NestedLoop,n,m,capacity,output),hash=estimate(Algorithm::HashJoin,n,m,capacity,output);
 return cost(nested,w)<=cost(hash,w)?Plan{Algorithm::NestedLoop,capacity,nested}:Plan{Algorithm::HashJoin,capacity,hash};
}
struct Result {std::vector<std::pair<int,int>> rows;Counts measured;};
inline Result execute(Plan p,const std::vector<int>&a,const std::vector<int>&b){
 if(!p.pageCapacity)throw std::invalid_argument("positive page capacity");
 Result result;
 auto read=[&](const std::vector<int>& data,std::size_t i){if(i%p.pageCapacity==0)++result.measured.pages;return data.at(i);};
 auto emit=[&](int x,int y){result.rows.emplace_back(x,y);++result.measured.outputs;};
 if(p.algorithm==Algorithm::NestedLoop){
  for(std::size_t i=0;i<a.size();++i){int x=read(a,i);for(std::size_t j=0;j<b.size();++j){int y=read(b,j);++result.measured.operations;if(x==y)emit(x,y);}}
 }else{
  std::unordered_multimap<int,int> table;
  for(std::size_t j=0;j<b.size();++j){int y=read(b,j);table.emplace(y,y);++result.measured.operations;}
  for(std::size_t i=0;i<a.size();++i){int x=read(a,i);++result.measured.operations;auto range=table.equal_range(x);for(auto it=range.first;it!=range.second;++it)emit(x,it->second);}
 }
 return result;
}
inline void require(bool ok){if(!ok)throw std::runtime_error("check failed");}
}

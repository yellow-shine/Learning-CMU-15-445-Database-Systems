#pragma once
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <vector>
namespace tutorial {
struct Row {int key; int value;};
using Rows=std::vector<Row>;
using Shards=std::vector<Rows>;
using Joined=std::vector<std::tuple<int,int,int>>;
inline void require_nodes(std::size_t p) {if(!p) throw std::invalid_argument("zero nodes");}
inline std::size_t route(int key,std::size_t p) {require_nodes(p); return static_cast<std::uint64_t>(key)%p;}
struct Exchange {Shards nodes; std::size_t sent=0;};
inline Exchange exchange(const Rows& rows,std::size_t p,bool broadcast) {
 require_nodes(p); Exchange x{Shards(p),0};
 for(auto row:rows) {
  if(broadcast) {for(auto& node:x.nodes) {node.push_back(row); ++x.sent;}}
  else {x.nodes[route(row.key,p)].push_back(row); ++x.sent;}
 }
 return x;
}
inline Joined local_join(const Rows& left,const Rows& right) {
 std::unordered_map<int,std::vector<int>> table;
 for(auto row:right) table[row.key].push_back(row.value);
 Joined out;
 for(auto row:left) {
  auto it=table.find(row.key);
  if(it!=table.end()) for(auto v:it->second) out.emplace_back(row.key,row.value,v);
 }
 return out;
}
struct JoinResult {Joined rows; std::size_t sent;};
inline JoinResult broadcast_join(const Rows& left,const Rows& right,std::size_t p) {
 require_nodes(p); Shards l(p); for(std::size_t i=0;i<left.size();++i) l[i%p].push_back(left[i]);
 auto r=exchange(right,p,true); JoinResult out{{},r.sent};
 for(std::size_t i=0;i<p;++i) {auto part=local_join(l[i],r.nodes[i]); out.rows.insert(out.rows.end(),part.begin(),part.end());}
 return out;
}
inline JoinResult repartition_join(const Rows& left,const Rows& right,std::size_t p) {
 auto l=exchange(left,p,false),r=exchange(right,p,false); JoinResult out{{},l.sent+r.sent};
 for(std::size_t i=0;i<p;++i) {auto part=local_join(l.nodes[i],r.nodes[i]); out.rows.insert(out.rows.end(),part.begin(),part.end());}
 return out;
}
struct Aggregate {
 std::int64_t sum=0; std::uint64_t count=0;
 double average() const {if(!count) throw std::logic_error("empty average"); return static_cast<double>(sum)/static_cast<double>(count);}
 bool operator==(const Aggregate& other) const {return sum==other.sum && count==other.count;}
};
inline void merge(Aggregate& into,const Aggregate& part) {
 if((part.sum>0 && into.sum>std::numeric_limits<std::int64_t>::max()-part.sum) ||
    (part.sum<0 && into.sum<std::numeric_limits<std::int64_t>::min()-part.sum) ||
    into.count>std::numeric_limits<std::uint64_t>::max()-part.count) throw std::overflow_error("aggregate overflow");
 into.sum+=part.sum; into.count+=part.count;
}
struct AggregateResult {std::map<int,Aggregate> groups; std::size_t sent=0;};
inline AggregateResult partial_aggregate(const Shards& shards,std::size_t reducers) {
 require_nodes(reducers);
 std::vector<std::vector<std::pair<int,Aggregate>>> messages(reducers);
 AggregateResult out;
 for(const auto& shard:shards) {
  std::unordered_map<int,Aggregate> partial;
  for(auto row:shard) merge(partial[row.key],{row.value,1});
  for(const auto& group:partial) {messages[route(group.first,reducers)].push_back(group); ++out.sent;}
 }
 for(const auto& node:messages) {
  std::unordered_map<int,Aggregate> combined;
  for(const auto& message:node) merge(combined[message.first],message.second);
  for(const auto& group:combined) out.groups.emplace(group);
 }
 return out;
}
}

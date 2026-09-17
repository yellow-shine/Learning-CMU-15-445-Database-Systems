#pragma once
#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace tutorial {

enum class Level { ReadUncommitted, ReadCommitted, RepeatableRead, Snapshot };
struct WouldBlock : std::runtime_error { WouldBlock():std::runtime_error("would block"){} };
class Isolation {
  struct Tx {
    bool active=true;
    std::uint64_t start=0;
    std::map<std::string,int> snapshot, writes;
  };
  Level level_;
  std::map<std::string,int> data_;
  std::map<std::string,std::uint64_t> versions_;
  std::map<int,Tx> txs_;
  std::map<std::string,int> exclusive_;
  std::map<std::string,std::set<int>> shared_;
  int next_=0;
  std::uint64_t clock_=0;
  Tx& active(int id) { auto& t=txs_.at(id); if(!t.active) throw std::logic_error("inactive"); return t; }
  void release(int id) {
    for(auto it=exclusive_.begin();it!=exclusive_.end();) {
      if(it->second==id)it=exclusive_.erase(it); else ++it;
    }
    for(auto& [key,readers]:shared_) { (void)key; readers.erase(id); }
  }
 public:
  Isolation(Level level,std::map<std::string,int> initial):level_(level),data_(std::move(initial)) {}
  int begin() {
    if(next_==std::numeric_limits<int>::max()) throw std::overflow_error("id");
    Tx t; t.start=clock_; if(level_==Level::Snapshot)t.snapshot=data_;
    txs_.emplace(++next_,std::move(t)); return next_;
  }
  std::optional<int> read(int id,const std::string& key) {
    auto& t=active(id); if(key.empty())throw std::invalid_argument("key");
    auto own=t.writes.find(key); if(own!=t.writes.end()) return own->second;
    if(level_==Level::RepeatableRead) {
      auto owner=exclusive_.find(key);
      if(owner!=exclusive_.end() && owner->second!=id)throw WouldBlock();
      // Record locks protect existing rows, not gaps/predicates.
      if(data_.count(key))shared_[key].insert(id);
    }
    if(level_==Level::ReadUncommitted) {
      auto owner=exclusive_.find(key);
      if(owner!=exclusive_.end())return txs_.at(owner->second).writes.at(key);
    }
    const auto& source=level_==Level::Snapshot?t.snapshot:data_;
    auto it=source.find(key); return it==source.end()?std::nullopt:std::optional<int>(it->second);
  }
  std::map<std::string,int> scan(int id) {
    auto& t=active(id); std::set<std::string> keys;
    const auto& source=level_==Level::Snapshot?t.snapshot:data_;
    for(const auto& [key,value]:source){(void)value;keys.insert(key);}
    for(const auto& [key,value]:t.writes){(void)value;keys.insert(key);}
    if(level_==Level::ReadUncommitted) for(const auto& [key,owner]:exclusive_){(void)owner; keys.insert(key);}
    std::map<std::string,int> result;
    for(const auto& key:keys){auto value=read(id,key);if(value)result[key]=*value;}
    return result;
  }
  void write(int id,const std::string& key,int value) {
    auto& t=active(id); if(key.empty())throw std::invalid_argument("key");
    if(level_!=Level::Snapshot) {
      auto owner=exclusive_.find(key);
      if(owner!=exclusive_.end() && owner->second!=id)throw WouldBlock();
      for(int reader:shared_[key])if(reader!=id)throw WouldBlock();
      exclusive_[key]=id;
    }
    t.writes[key]=value;
  }
  void abort(int id) {auto& t=active(id);release(id);t.writes.clear();t.active=false;}
  bool commit(int id) {
    auto& t=active(id);
    if(level_==Level::Snapshot)for(const auto& [key,value]:t.writes) {
      (void)value;auto it=versions_.find(key);
      if(it!=versions_.end() && it->second>t.start){abort(id);return false;}
    }
    if(clock_==std::numeric_limits<std::uint64_t>::max())throw std::overflow_error("clock");
    auto candidate=data_;auto versions=versions_;
    for(const auto& [key,value]:t.writes){candidate[key]=value;versions[key]=clock_+1;}
    data_.swap(candidate);versions_.swap(versions);++clock_;
    release(id);t.writes.clear();t.active=false;return true;
  }
};
struct Anomaly {std::string name; int first, second;};
inline std::vector<Anomaly> anomalies() {
  std::vector<Anomaly> result;
  {Isolation d(Level::ReadUncommitted,{{"x",0}});int a=d.begin(),b=d.begin();
   d.write(a,"x",9);int dirty=*d.read(b,"x");d.abort(a);
   result.push_back({"dirty read",dirty,*d.read(b,"x")});d.commit(b);}
  {Isolation d(Level::ReadCommitted,{{"x",0}});int a=d.begin(),b=d.begin();int first=*d.read(a,"x");
   d.write(b,"x",1);d.commit(b);result.push_back({"nonrepeatable read",first,*d.read(a,"x")});d.commit(a);}
  {Isolation d(Level::RepeatableRead,{{"a",1}});int a=d.begin(),b=d.begin();int first=static_cast<int>(d.scan(a).size());
   d.write(b,"b",1);d.commit(b);result.push_back({"phantom",first,static_cast<int>(d.scan(a).size())});d.commit(a);}
  {Isolation d(Level::ReadCommitted,{{"x",0}});int a=d.begin(),b=d.begin();int x=*d.read(a,"x"),y=*d.read(b,"x");
   d.write(a,"x",x+1);d.commit(a);d.write(b,"x",y+1);d.commit(b);int c=d.begin();
   result.push_back({"lost update (expected/actual)",2,*d.read(c,"x")});d.commit(c);}
  {Isolation d(Level::Snapshot,{{"a",1},{"b",1}});int a=d.begin(),b=d.begin();
   if(d.read(a,"b")==1)d.write(a,"a",0);
   if(d.read(b,"a")==1)d.write(b,"b",0);
   d.commit(a);d.commit(b);int c=d.begin();
   result.push_back({"write skew (on-call before/after)",2,*d.read(c,"a")+*d.read(c,"b")});d.commit(c);}
  return result;
}

}

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
// Full before-images form the undo chain for a one-column optional tuple.
struct Undo { std::uint64_t timestamp; std::optional<int> value; };
struct Row {
  std::uint64_t timestamp=0;
  std::optional<int> value;
  std::vector<Undo> undo;
  std::optional<int> at(std::uint64_t snapshot) const {
    auto ts=timestamp; auto result=value;
    for(auto it=undo.rbegin(); ts>snapshot && it!=undo.rend(); ++it) {
      ts=it->timestamp; result=it->value;
    }
    return ts<=snapshot ? result : std::nullopt;
  }
};
class MVCC {
  struct Tx {
    std::uint64_t snapshot;
    bool active=true;
    std::set<std::string> reads;
    bool scanned=false;
    std::map<std::string,std::optional<int>> writes;
  };
  std::map<std::string,Row> rows_;
  std::map<int,Tx> transactions_;
  int next_=0;
  std::uint64_t clock_=0;
  Tx& active(int id) {
    auto& t=transactions_.at(id); if(!t.active) throw std::logic_error("inactive transaction"); return t;
  }
 public:
  explicit MVCC(const std::map<std::string,int>& initial) {
    for(const auto& [key,value]:initial) {
      if(key.empty()) throw std::invalid_argument("key");
      rows_.emplace(key,Row{0,value,{}});
    }
  }
  int begin() {
    if(next_==std::numeric_limits<int>::max()) throw std::overflow_error("id");
    transactions_.emplace(++next_,Tx{clock_,true,{},false,{}}); return next_;
  }
  std::optional<int> read(int id,const std::string& key) {
    auto& t=active(id); if(key.empty()) throw std::invalid_argument("key");
    t.reads.insert(key);
    auto own=t.writes.find(key); if(own!=t.writes.end()) return own->second;
    auto row=rows_.find(key); return row==rows_.end()?std::nullopt:row->second.at(t.snapshot);
  }
  void write(int id,const std::string& key,std::optional<int> value) {
    auto& t=active(id); if(key.empty()) throw std::invalid_argument("key"); t.writes[key]=value;
  }

  std::map<std::string,int> scan(int id) {
    auto& t=active(id); t.scanned=true; std::map<std::string,int> result;
    for(const auto& [key,row]:rows_) {
      auto value=row.at(t.snapshot); if(value) result[key]=*value;
    }
    for(const auto& [key,value]:t.writes) {
      if(value) result[key]=*value; else result.erase(key);
    }
    return result;
  }
  void abort(int id) { auto& t=active(id); t.writes.clear(); t.active=false; }
  bool commit(int id) {
    auto& t=active(id);
    // A table-wide predicate token deliberately over-approximates range conflicts.
    if(t.scanned && clock_>t.snapshot) { abort(id); return false; }
    for(const auto& key:t.reads) {
      auto row=rows_.find(key);
      if(row!=rows_.end() && row->second.timestamp>t.snapshot) { abort(id); return false; }
    }
    for(const auto& [key,value]:t.writes) {
      (void)value; auto row=rows_.find(key);
      if(row!=rows_.end() && row->second.timestamp>t.snapshot) { abort(id); return false; }
    }
    if(!t.writes.empty()) {
      if(clock_==std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("clock");
      // ponytail: copy the table for exception-safe atomic publication; use a prepared write batch at scale.
      auto candidate=rows_; const auto timestamp=clock_+1;
      for(const auto& [key,value]:t.writes) {
        auto& row=candidate[key]; row.undo.push_back({row.timestamp,row.value});
        row.timestamp=timestamp; row.value=value;
      }
      rows_.swap(candidate); clock_=timestamp;
    }
    t.writes.clear(); t.active=false; return true;
  }
};

}

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

class TimestampOrdering {
  struct Item { int value; std::uint64_t read_ts=0, write_ts=0; };
  struct Tx { bool active=true; std::map<std::string,int> writes; };
  std::uint64_t clock_=0;
  std::map<std::string,Item> items_;
  std::map<std::uint64_t,Tx> transactions_;
  Tx& active(std::uint64_t ts) {
    auto& t=transactions_.at(ts); if(!t.active) throw std::logic_error("inactive"); return t;
  }
  bool writable(std::uint64_t ts, const Item& item) const {
    return ts>=item.read_ts && ts>=item.write_ts;
  }
 public:
  explicit TimestampOrdering(const std::map<std::string,int>& initial) {
    for(const auto& [key,value]:initial) items_.emplace(key,Item{value});
  }
  std::uint64_t begin() {
    if(clock_==std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("clock");
    transactions_.emplace(++clock_,Tx{}); return clock_;
  }
  void abort(std::uint64_t ts) { auto& t=active(ts); t.writes.clear(); t.active=false; }
  std::optional<int> read(std::uint64_t ts, const std::string& key) {
    auto& t=active(ts); auto& item=items_.at(key);
    if(ts<item.write_ts) { abort(ts); return std::nullopt; }
    item.read_ts=std::max(item.read_ts,ts);
    auto own=t.writes.find(key); return own==t.writes.end()?item.value:own->second;
  }
  bool write(std::uint64_t ts,const std::string& key,int value) {
    auto& t=active(ts); const auto& item=items_.at(key);
    if(!writable(ts,item)) { abort(ts); return false; }
    t.writes[key]=value; return true;
  }
  bool commit(std::uint64_t ts) {
    auto& t=active(ts);
    // Deferred publication: revalidate every write before changing any value.
    for(const auto& [key,value]:t.writes) {
      (void)value; if(!writable(ts,items_.at(key))) { abort(ts); return false; }
    }
    for(const auto& [key,value]:t.writes) { auto& item=items_.at(key); item.value=value; item.write_ts=ts; }
    t.writes.clear(); t.active=false; return true;
  }
};

}

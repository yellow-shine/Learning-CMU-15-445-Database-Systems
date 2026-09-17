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

class OCC {
  struct Item { int value; std::uint64_t version=0; };
  struct Observation { int value; std::uint64_t version; };
  struct Tx {
    enum class Phase { Read, Validate, Write, Committed, Aborted } phase=Phase::Read;
    std::map<std::string,Observation> reads;
    std::map<std::string,int> writes;
  };
  std::map<std::string,Item> items_;
  std::map<int,Tx> transactions_;
  int next_=0;
  Tx& reading(int id) {
    auto& t=transactions_.at(id);
    if(t.phase!=Tx::Phase::Read) throw std::logic_error("not in read phase"); return t;
  }
 public:
  explicit OCC(const std::map<std::string,int>& initial) {
    for(const auto& [key,value]:initial) items_.emplace(key,Item{value});
  }
  int begin() {
    if(next_==std::numeric_limits<int>::max()) throw std::overflow_error("id");
    transactions_.emplace(++next_,Tx{}); return next_;
  }
  int read(int id,const std::string& key) {
    auto& t=reading(id); auto& item=items_.at(key);
    auto own=t.writes.find(key); if(own!=t.writes.end()) return own->second;
    auto [it,inserted]=t.reads.emplace(key,Observation{item.value,item.version});
    (void)inserted; return it->second.value;
  }
  void write(int id,const std::string& key,int value) {
    read(id,key); // Observe even blind writes, preventing concurrent overwrite.
    reading(id).writes[key]=value;
  }
  void abort(int id) { auto& t=reading(id); t.phase=Tx::Phase::Aborted; t.writes.clear(); }
  bool commit(int id) {
    auto& t=reading(id); t.phase=Tx::Phase::Validate;
    for(const auto& [key,seen]:t.reads) if(items_.at(key).version!=seen.version) {
      t.phase=Tx::Phase::Aborted; t.writes.clear(); return false;
    }
    for(const auto& [key,value]:t.writes) {
      (void)value;
      if(items_.at(key).version==std::numeric_limits<std::uint64_t>::max()) {
        t.phase=Tx::Phase::Aborted; throw std::overflow_error("version");
      }
    }
    t.phase=Tx::Phase::Write;
    for(const auto& [key,value]:t.writes) { auto& item=items_.at(key); item.value=value; ++item.version; }
    t.writes.clear(); t.phase=Tx::Phase::Committed; return true;
  }
};

}

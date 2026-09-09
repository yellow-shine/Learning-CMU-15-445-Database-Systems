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

class Deadlocks {
 public:
  enum class State { Active, Committed, Aborted };
 private:
  std::map<int,State> states_;
  std::map<std::string,int> owners_;
  std::map<int,std::string> waiting_;
  void active(int id) const { if (states_.at(id)!=State::Active) throw std::logic_error("inactive"); }
  void release(int id, State state) {
    active(id); states_[id]=state; waiting_.erase(id);
    for(auto it=owners_.begin(); it!=owners_.end();) {
      if(it->second==id) it=owners_.erase(it); else ++it;
    }
  }
 public:
  void begin(int id) {
    if(id<0 || !states_.emplace(id,State::Active).second) throw std::invalid_argument("id");
  }
  State state(int id) const { return states_.at(id); }
  bool request(int id, const std::string& key) {
    active(id); if(key.empty()) throw std::invalid_argument("key");
    auto pending=waiting_.find(id);
    if(pending!=waiting_.end() && pending->second!=key) throw std::logic_error("one pending request");
    auto owner=owners_.find(key);
    if(owner==owners_.end() || owner->second==id) {
      owners_[key]=id; waiting_.erase(id); return true;
    }
    waiting_[id]=key; return false;
  }
  std::map<int,int> waits_for() const {
    std::map<int,int> graph;
    for(const auto& [id,key]:waiting_) {
      auto owner=owners_.find(key);
      if(owner!=owners_.end() && owner->second!=id) graph[id]=owner->second;
    }
    return graph;
  }
  std::vector<int> cycle() const {
    auto graph=waits_for(); std::set<int> finished;
    for(const auto& [start, ignored]:graph) {
      (void)ignored; std::vector<int> path; std::map<int,std::size_t> position;
      int v=start;
      while(!finished.count(v)) {
        auto seen=position.find(v);
        if(seen!=position.end()) return {path.begin()+static_cast<std::ptrdiff_t>(seen->second),path.end()};
        position[v]=path.size(); path.push_back(v);
        auto edge=graph.find(v); if(edge==graph.end()) break; v=edge->second;
      }
      finished.insert(path.begin(),path.end());
    }
    return {};
  }
  std::optional<int> resolve_one() {
    auto found=cycle(); if(found.empty()) return std::nullopt;
    int victim=*std::max_element(found.begin(),found.end());
    release(victim,State::Aborted); return victim;
  }
  void commit(int id) {
    active(id); if(waiting_.count(id)) throw std::logic_error("pending request");
    release(id,State::Committed);
  }
  void abort(int id) { release(id,State::Aborted); }
};

}

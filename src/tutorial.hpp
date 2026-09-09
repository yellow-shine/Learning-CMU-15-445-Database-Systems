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

enum class Mode { Shared, Exclusive };
class LockManager {
  struct Tx { bool growing=true; bool active=true; };
  std::map<int,Tx> transactions_;
  std::map<std::string,std::map<int,Mode>> locks_;
  bool strict_;
  Tx& active(int id) {
    auto& t=transactions_.at(id);
    if (!t.active) throw std::logic_error("finished transaction");
    return t;
  }
 public:
  explicit LockManager(bool strict) : strict_(strict) {}
  void begin(int id) {
    if (id<0 || !transactions_.emplace(id,Tx{}).second) throw std::invalid_argument("transaction id");
  }
  bool acquire(int id, const std::string& key, Mode mode) {
    auto& t=active(id);
    if (key.empty()) throw std::invalid_argument("empty key");
    auto it=locks_.find(key);
    if (it!=locks_.end()) {
      auto mine=it->second.find(id);
      if (mine!=it->second.end() && (mine->second==Mode::Exclusive || mode==Mode::Shared)) return true;
    }
    if (!t.growing) throw std::logic_error("acquire or upgrade after unlock");
    if (it!=locks_.end()) for (const auto& [owner, held] : it->second)
      if (owner!=id && (held==Mode::Exclusive || mode==Mode::Exclusive)) return false;
    locks_[key][id]=mode; return true;
  }
  void unlock(int id, const std::string& key) {
    auto& t=active(id); auto& owners=locks_.at(key); auto held=owners.at(id);
    if (strict_ && held==Mode::Exclusive) throw std::logic_error("strict X retained until finish");
    owners.erase(id); if (owners.empty()) locks_.erase(key); t.growing=false;
  }
  void finish(int id) {
    auto& t=active(id);
    for (auto it=locks_.begin(); it!=locks_.end();) {
      it->second.erase(id); if (it->second.empty()) it=locks_.erase(it); else ++it;
    }
    t.active=false;
  }
};

}

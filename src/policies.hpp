#pragma once
#include <array>
#include <cstddef>
#include <stdexcept>
#include <vector>
namespace tiny {
inline void require(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
struct Update { std::size_t tx, page; int before, after; };
// Deterministic in-memory crash model, not a disk or concurrency implementation.
class BufferModel {
  std::array<std::size_t,2> owner{};
  std::array<bool,3> active{}, committed{}, used{};
  std::vector<Update> log;
public:
  const bool steal, force;
  std::array<int,2> memory{}, disk{};
  std::size_t writes=0;
  BufferModel(bool s, bool f) : steal(s), force(f) {}
  void begin(std::size_t tx) { require(tx>0 && tx<3 && !used[tx],"invalid transaction"); active[tx]=used[tx]=true; }
  void update(std::size_t tx, std::size_t page, int value) {
    require(tx>0 && tx<3 && active[tx] && page<2,"invalid update");
    require(!owner[page] || owner[page]==tx,"write conflict");
    log.push_back({tx,page,memory[page],value}); owner[page]=tx; memory[page]=value;
  }
  bool evict(std::size_t page) {
    require(page<2,"invalid page");
    if (!steal && owner[page]) return false;
    disk[page]=memory[page]; ++writes; return true;
  }
  void commit(std::size_t tx) {
    require(tx>0 && tx<3 && active[tx],"invalid commit");
    committed[tx]=true; active[tx]=false; // Commit decision represents durable log.
    for (std::size_t p=0;p<2;++p) if (owner[p]==tx) {
      owner[p]=0;
      if (force) { disk[p]=memory[p]; ++writes; }
    }
  }
  void crash() { memory=disk; }
  void recover(bool undo, bool redo) {
    if (redo) for (auto r:log) if(committed[r.tx]) disk[r.page]=r.after;
    if (undo) for(auto it=log.rbegin();it!=log.rend();++it)
      if(!committed[it->tx]) disk[it->page]=it->before;
    memory=disk;
  }
};
}

#pragma once
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <fcntl.h>
#include <unistd.h>

namespace tiny {
using Word = std::uint64_t;
inline void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
struct Fd {
  int value;
  explicit Fd(const std::string& path, int flags) : value(::open(path.c_str(), flags, 0600)) {
    require(value >= 0, "open failed");
  }
  ~Fd() { ::close(value); }
  Fd(const Fd&) = delete;
  Fd& operator=(const Fd&) = delete;
};
inline void sync(int fd) { require(::fsync(fd) == 0, "fsync failed"); }
inline void writeAll(int fd, const unsigned char* p, std::size_t n) {
  while (n) {
    auto count = ::write(fd, p, n);
    if (count < 0 && errno == EINTR) continue;
    require(count > 0, "write failed");
    p += count; n -= static_cast<std::size_t>(count);
  }
}
inline std::vector<unsigned char> readAll(int fd) {
  std::vector<unsigned char> result;
  std::array<unsigned char, 4096> block{};
  for (;;) {
    auto n = ::read(fd, block.data(), block.size());
    if (n < 0 && errno == EINTR) continue;
    require(n >= 0, "read failed");
    if (!n) return result;
    result.insert(result.end(), block.begin(), block.begin() + n);
  }
}
inline std::vector<unsigned char> encode(const std::vector<Word>& words) {
  std::vector<unsigned char> bytes;
  for (auto w : words) for (int i = 0; i < 8; ++i) bytes.push_back(static_cast<unsigned char>(w >> (i*8)));
  return bytes;
}
inline Word word(const unsigned char* bytes) {
  Word w = 0;
  for (int i = 0; i < 8; ++i) w |= Word(bytes[i]) << (i*8);
  return w;
}
inline Word checksum(const unsigned char* bytes, std::size_t n) {
  Word h = 14695981039346656037ULL;
  for (std::size_t i = 0; i < n; ++i) h = (h ^ bytes[i]) * 1099511628211ULL;
  return h;
}
enum Kind : Word { Update = 1, Commit = 2, End = 3, Clr = 4 };
struct Record { Word kind, lsn, tx, page, before, after, prev, next; };
constexpr std::size_t recordBytes = 72;
inline std::vector<unsigned char> pack(const Record& r) {
  auto bytes = encode({r.kind,r.lsn,r.tx,r.page,r.before,r.after,r.prev,r.next});
  auto hash = encode({checksum(bytes.data(), bytes.size())});
  bytes.insert(bytes.end(), hash.begin(), hash.end());
  return bytes;
}
class Log {
  Fd fd;
public:
  std::vector<Record> records;
  Word durable = 0;
  bool trimmedTail = false;
  explicit Log(const std::string& path) : fd(path, O_CREAT | O_RDWR) {
    auto bytes = readAll(fd.value);
    std::map<Word,Word> last; std::set<Word> ended;
    for (std::size_t offset = 0; offset + recordBytes <= bytes.size(); offset += recordBytes) {
      auto p = bytes.data() + offset;
      require(word(p+64) == checksum(p,64), "corrupt log checksum (not a torn tail)");
      Record r{word(p),word(p+8),word(p+16),word(p+24),word(p+32),word(p+40),word(p+48),word(p+56)};
      require(r.lsn == records.size()+1 && r.kind >= Update && r.kind <= Clr && r.tx > 0 && r.page < 4 && r.prev < r.lsn && r.next < r.lsn, "invalid log fields");
      if (r.prev) require(records.at(r.prev-1).tx == r.tx, "invalid transaction chain");
      require(!ended.count(r.tx) && r.prev==last[r.tx], "broken transaction history");
      if(r.kind==Clr) {
        require(r.before>0 && r.before<r.lsn && r.prev>0,"CLR target");
        auto target=records.at(r.before-1); auto previous=records.at(r.prev-1);
        require(target.kind==Update && target.tx==r.tx && target.page==r.page && target.before==r.after && target.prev==r.next,"CLR undoNext/image");
        require(r.before==(previous.kind==Clr?previous.next:previous.lsn),"CLR must undo next outstanding update");
      } else require(r.next==0,"non-CLR undoNext");
      if(r.kind==Commit || r.kind==End) {
        require(r.page==0 && r.before==0 && r.after==0,"terminal fields");
        if(r.kind==End) require(r.prev>0 && records.at(r.prev-1).kind==Clr && records.at(r.prev-1).next==0,"premature End");
        ended.insert(r.tx);
      }
      if(r.kind==Update && r.prev) require(records.at(r.prev-1).kind==Update,"update during rollback");
      last[r.tx]=r.lsn;
      records.push_back(r);
    }
    auto valid = records.size()*recordBytes;
    trimmedTail = valid != bytes.size();
    if (trimmedTail) { require(::ftruncate(fd.value, static_cast<off_t>(valid)) == 0, "truncate failed"); sync(fd.value); }
    require(::lseek(fd.value, 0, SEEK_END) >= 0, "seek failed");
    durable = records.size();
  }
  Word append(Record r) {
    r.lsn = records.size()+1;
    auto bytes = pack(r); writeAll(fd.value, bytes.data(), bytes.size());
    records.push_back(r); return r.lsn;
  }
  void flush() { sync(fd.value); durable = records.size(); }
};
struct Page { Word value = 0, lsn = 0; };
class Store {
  std::string directory;
  std::map<Word,Word> active;
  std::set<Word> used;
  std::map<Word,Word> owners;
  bool recoveryRequired=false;
public:
  Log log;
  std::array<Page,4> pages{};
  explicit Store(const std::string& dir) : directory(dir), log(dir+"/wal") {
    // Persist directory entry as well as file contents for newly created logs.
    Fd directoryFd(dir, O_RDONLY); sync(directoryFd.value);
    if (std::filesystem::exists(dir+"/pages")) {
      Fd fd(dir+"/pages", O_RDONLY); auto bytes = readAll(fd.value);
      require(bytes.size() == 72 && word(bytes.data()+64) == checksum(bytes.data(),64), "corrupt pages");
      for (std::size_t i=0;i<4;++i) { pages[i] = {word(bytes.data()+i*16),word(bytes.data()+i*16+8)}; require(pages[i].lsn <= log.records.size(), "page ahead of WAL"); if(pages[i].lsn) {
        auto r=log.records.at(pages[i].lsn-1);
        require((r.kind==Update || r.kind==Clr) && r.page==i && r.after==pages[i].value,"invalid PageLSN/image");
      } else require(pages[i].value==0,"nonzero genesis page"); }
    }
    for (const auto& r : log.records) used.insert(r.tx);
    recoveryRequired=!log.records.empty();
  }
  void begin(Word tx) { require(!recoveryRequired,"recover before accepting transactions"); require(tx > 0 && !used.count(tx), "transaction id reused"); used.insert(tx); active[tx]=0; }
  Word update(Word tx, Word page, Word value) {
    require(active.count(tx) && page < pages.size(), "invalid update");
    require(!owners.count(page) || owners[page] == tx, "write conflict: strict ownership");
    owners[page]=tx;
    auto lsn = log.append({Update,0,tx,page,pages[page].value,value,active[tx],0});
    active[tx]=lsn; pages[page]={value,lsn}; return lsn;
  }
  void commit(Word tx) {
    require(active.count(tx), "unknown transaction");
    log.append({Commit,0,tx,0,0,0,active[tx],0}); log.flush(); active.erase(tx);
    for (auto it=owners.begin(); it!=owners.end();) { if (it->second == tx) it=owners.erase(it); else ++it; }
  }
  void flushPages() {
    log.flush(); // WAL rule: every PageLSN must be durable before data write.
    std::vector<Word> words;
    for (auto p : pages) { require(p.lsn <= log.durable, "WAL violation"); words.push_back(p.value); words.push_back(p.lsn); }
    auto bytes=encode(words); auto hash=encode({checksum(bytes.data(),bytes.size())}); bytes.insert(bytes.end(),hash.begin(),hash.end());
    { Fd fd(directory+"/pages.tmp", O_CREAT|O_TRUNC|O_WRONLY); writeAll(fd.value,bytes.data(),bytes.size()); sync(fd.value); }
    require(::rename((directory+"/pages.tmp").c_str(),(directory+"/pages").c_str()) == 0,"rename failed");
    Fd fd(directory,O_RDONLY); sync(fd.value);
  }
  std::size_t redone=0, undone=0, skipped=0;
  std::map<Word,Word> transactionTable, dirtyPageTable;
  void recover(const std::function<void(const char*)>& hook = {}) {
    require(active.empty(),"cannot recover active instance"); recoveryRequired=true;
    redone=undone=skipped=0; transactionTable.clear(); dirtyPageTable.clear();
    // Analysis reconstructs loser lastLSNs and conservative recLSNs from genesis.
    for(auto r:log.records) {
      if(r.kind==Commit || r.kind==End) transactionTable.erase(r.tx);
      else { transactionTable[r.tx]=r.lsn; dirtyPageTable.emplace(r.page,r.lsn); }
    }
    if(hook) hook("analysis");
    Word start=log.records.size()+1;
    for(auto e:dirtyPageTable) if(e.second<start) start=e.second;
    // Repeat history, including loser updates and redo-only CLRs. Never reset pages.
    for(std::size_t i=static_cast<std::size_t>(start-1);i<log.records.size();++i) {
      auto r=log.records[i];
      if(r.kind!=Update && r.kind!=Clr) continue;
      if(r.lsn>=dirtyPageTable.at(r.page) && pages[r.page].lsn<r.lsn) {
        pages[r.page]={r.after,r.lsn}; ++redone;
      } else ++skipped;
    }
    if(hook) hook("redo");
    // Ordered frontier follows each transaction's chain, globally greatest LSN first.
    std::map<Word,Word> frontier;
    for(auto e:transactionTable) frontier[e.second]=e.first;
    while(!frontier.empty()) {
      auto it=std::prev(frontier.end()); Word lsn=it->first, tx=it->second; frontier.erase(it);
      auto r=log.records.at(lsn-1); Word next=0;
      if(r.kind==Clr) next=r.next; // CLRs are redo-only; do not undo an undo.
      else {
        require(r.kind==Update,"undo chain must contain Update or CLR");
        next=r.prev;
        auto clr=log.append({Clr,0,tx,r.page,r.lsn,r.before,transactionTable.at(tx),next});
        transactionTable[tx]=clr; log.flush();
        if(hook) hook("clr"); // Durable CLR but its page may still be stale.
        pages[r.page]={r.before,clr}; ++undone;
        flushPages();
        if(hook) hook("undo");
      }
      if(next) frontier[next]=tx;
      else {
        log.append({End,0,tx,0,0,0,transactionTable.at(tx),0}); log.flush(); transactionTable.erase(tx);
        if(hook) hook("end");
      }
    }
    flushPages();
    if(hook) hook("pages");
    active.clear(); owners.clear(); recoveryRequired=false;
  }
};
}

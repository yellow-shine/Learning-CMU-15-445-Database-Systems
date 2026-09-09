#pragma once
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
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
enum Kind : Word { Update = 1, Commit = 2 };
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
    for (std::size_t offset = 0; offset + recordBytes <= bytes.size(); offset += recordBytes) {
      auto p = bytes.data() + offset;
      require(word(p+64) == checksum(p,64), "corrupt log checksum (not a torn tail)");
      Record r{word(p),word(p+8),word(p+16),word(p+24),word(p+32),word(p+40),word(p+48),word(p+56)};
      require(r.lsn == records.size()+1 && r.kind >= Update && r.kind <= Commit && r.tx > 0 && r.page < 4 && r.prev < r.lsn && r.next == 0, "invalid log fields");
      if (r.prev) require(records.at(r.prev-1).tx == r.tx, "invalid transaction chain");
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
  std::map<Word,Word> dirty;
public:
  Log log;
  std::array<Page,4> pages{};
  explicit Store(const std::string& dir) : directory(dir), log(dir+"/wal") {
    // Persist directory entry as well as file contents for newly created logs.
    Fd directoryFd(dir, O_RDONLY); sync(directoryFd.value);
    if (std::filesystem::exists(dir+"/pages")) {
      Fd fd(dir+"/pages", O_RDONLY); auto bytes = readAll(fd.value);
      require(bytes.size() == 72 && word(bytes.data()+64) == checksum(bytes.data(),64), "corrupt pages");
      for (std::size_t i=0;i<4;++i) { pages[i] = {word(bytes.data()+i*16),word(bytes.data()+i*16+8)}; require(pages[i].lsn <= log.records.size(), "page ahead of WAL"); }
    }
    for (const auto& r : log.records) used.insert(r.tx);
  }
  void begin(Word tx) { require(tx > 0 && !used.count(tx), "transaction id reused"); used.insert(tx); active[tx]=0; }
  Word update(Word tx, Word page, Word value) {
    require(active.count(tx) && page < pages.size(), "invalid update");
    require(!owners.count(page) || owners[page] == tx, "write conflict: strict ownership");
    owners[page]=tx;
    auto lsn = log.append({Update,0,tx,page,pages[page].value,value,active[tx],0});
    active[tx]=lsn; dirty.emplace(page,lsn); pages[page]={value,lsn}; return lsn;
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
    Fd fd(directory,O_RDONLY); sync(fd.value); dirty.clear();
  }
  struct Checkpoint {
    Word cutoff=0, start=1;
    std::array<Page,4> base{};
    std::map<Word,Word> transactions, dirtyPages;
  };
  Word recoveryStart=1;
  std::size_t replayScanned=0;
  Word safeStart(const Checkpoint& cp) const {
    Word start=cp.cutoff+1;
    for(auto entry:cp.dirtyPages) start=std::min(start,entry.second);
    for(auto entry:cp.transactions) {
      Word lsn=entry.second;
      while(lsn) { start=std::min(start,lsn); lsn=log.records.at(lsn-1).prev; }
    }
    return start;
  }
  void checkpoint(const std::function<void()>& beforePublish = {}) {
    Checkpoint cp; cp.cutoff=log.records.size(); cp.transactions=active; cp.dirtyPages=dirty;
    // ponytail: committed base construction scans the retained log, not a fuzzy checkpoint.
    std::set<Word> winners;
    for(auto r:log.records) if(r.kind==Commit) winners.insert(r.tx);
    for(auto r:log.records) if(r.kind==Update && winners.count(r.tx)) cp.base[r.page]={r.after,r.lsn};
    cp.start=safeStart(cp); log.flush();
    std::vector<Word> words{0x43504b31,cp.cutoff,cp.start,cp.transactions.size(),cp.dirtyPages.size()};
    for(auto p:cp.base) {words.push_back(p.value); words.push_back(p.lsn);}
    for(auto e:cp.transactions) {words.push_back(e.first); words.push_back(e.second);}
    for(auto e:cp.dirtyPages) {words.push_back(e.first); words.push_back(e.second);}
    auto bytes=encode(words); auto hash=encode({checksum(bytes.data(),bytes.size())}); bytes.insert(bytes.end(),hash.begin(),hash.end());
    { Fd fd(directory+"/checkpoint.tmp",O_CREAT|O_TRUNC|O_WRONLY); writeAll(fd.value,bytes.data(),bytes.size()); sync(fd.value); }
    if(beforePublish) beforePublish();
    require(::rename((directory+"/checkpoint.tmp").c_str(),(directory+"/checkpoint").c_str())==0,"checkpoint rename failed");
    Fd fd(directory,O_RDONLY); sync(fd.value);
  }
  Checkpoint loadCheckpoint() const {
    Checkpoint cp;
    if(!std::filesystem::exists(directory+"/checkpoint")) return cp;
    Fd fd(directory+"/checkpoint",O_RDONLY); auto bytes=readAll(fd.value);
    require(bytes.size()>=112 && bytes.size()%8==0,"checkpoint length");
    require(word(bytes.data()+bytes.size()-8)==checksum(bytes.data(),bytes.size()-8),"checkpoint checksum");
    std::vector<Word> w; for(std::size_t i=0;i<bytes.size()-8;i+=8) w.push_back(word(bytes.data()+i));
    require(w[0]==0x43504b31 && w[1]<=log.records.size() && w[3]<=(w.size()-13)/2 && w[4]<=4 && w.size()==13+2*w[3]+2*w[4],"checkpoint fields");
    cp.cutoff=w[1]; cp.start=w[2];
    for(std::size_t i=0;i<4;++i) {cp.base[i]={w[5+2*i],w[6+2*i]}; require(cp.base[i].lsn<=cp.cutoff,"checkpoint page LSN");}
    std::size_t i=13;
    for(Word n=0;n<w[3];++n,i+=2) {
      require(w[i]>0 && w[i+1]<=cp.cutoff && cp.transactions.emplace(w[i],w[i+1]).second,"checkpoint TT");
      if(w[i+1]) require(log.records.at(w[i+1]-1).tx==w[i] && log.records.at(w[i+1]-1).kind==Update,"checkpoint lastLSN");
    }
    for(Word n=0;n<w[4];++n,i+=2) {
      require(w[i]<4 && w[i+1]>0 && w[i+1]<=cp.cutoff && cp.dirtyPages.emplace(w[i],w[i+1]).second,"checkpoint DPT");
      auto r=log.records.at(w[i+1]-1); require(r.kind==Update && r.page==w[i],"checkpoint recLSN");
    }
    require(cp.start==safeStart(cp),"unsafe checkpoint start"); return cp;
  }
  void recover() {
    auto cp=loadCheckpoint(); pages=cp.base; recoveryStart=cp.start; replayScanned=0;
    std::set<Word> candidates, winners;
    for(auto e:cp.transactions) candidates.insert(e.first);
    for(std::size_t i=static_cast<std::size_t>(cp.cutoff);i<log.records.size();++i) {
      auto r=log.records[i]; candidates.insert(r.tx); if(r.kind==Commit) winners.insert(r.tx);
    }
    for(std::size_t i=static_cast<std::size_t>(cp.start-1);i<log.records.size();++i) {
      ++replayScanned; auto r=log.records[i];
      if(r.kind==Update && candidates.count(r.tx) && winners.count(r.tx)) pages[r.page]={r.after,r.lsn};
    }
    flushPages(); active.clear(); owners.clear();
  }
};
}

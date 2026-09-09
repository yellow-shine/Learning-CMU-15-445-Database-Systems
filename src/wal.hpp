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
enum Kind : Word { Update = 1, Commit = 2, Abort = 3 };
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
      require(r.lsn == records.size()+1 && r.kind >= Update && r.kind <= Abort && r.tx > 0 && r.page < 4 && r.prev < r.lsn && r.next == 0, "invalid log fields");
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
  std::size_t redone=0, undone=0;
  void recover(const std::function<void(const char*)>& hook = {}) {
    // ponytail: rebuild from genesis rather than trusting partially undone disk pages.
    pages={}; redone=undone=0;
    std::set<Word> committed, aborted;
    std::map<Word,Word> losers;
    for(auto r:log.records) {
      if(r.kind==Commit) committed.insert(r.tx);
      if(r.kind==Abort) aborted.insert(r.tx);
    }
    for(auto r:log.records) if(r.kind==Update && !aborted.count(r.tx)) {
      pages[r.page]={r.after,r.lsn}; ++redone;
      if(!committed.count(r.tx)) losers[r.tx]=r.lsn;
    }
    if(hook) hook("redo");
    for(auto it=log.records.rbegin();it!=log.records.rend();++it) {
      if(it->kind==Update && losers.count(it->tx)) {
        pages[it->page]={it->before,0}; ++undone;
        if(hook) hook("undo");
      }
    }
    flushPages();
    if(hook) hook("pages");
    // End markers exclude old losers when future committed updates reuse their pages.
    for(auto e:losers) log.append({Abort,0,e.first,0,0,0,e.second,0});
    log.flush(); active.clear(); owners.clear();
  }
};
}

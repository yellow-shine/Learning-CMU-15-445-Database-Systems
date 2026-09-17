#pragma once
#include <array>
#include <cerrno>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>
#include <fcntl.h>
#include <unistd.h>
namespace tutorial {
inline void io_error(const char* what) {throw std::system_error(errno,std::generic_category(),what);}
class Fd {
 public:
  int value;
  explicit Fd(int fd):value(fd) {if(fd<0) io_error("open");}
  ~Fd(){::close(value);}
  Fd(const Fd&)=delete;
  Fd& operator=(const Fd&)=delete;
};
inline void sync_fd(int fd) {while(::fsync(fd)<0) {if(errno!=EINTR) io_error("fsync");}}
class Journal {
  Fd fd_;
  bool coordinator_, poisoned_=false;
  std::string records_;
  bool valid(const std::string& s) const {
    if(coordinator_) return s.empty() || s=="C" || s=="A";
    return s.empty() || s=="P" || s=="PC" || s=="PA" || s=="A";
  }
  static int open_log(const std::filesystem::path& path) {
    int fd=::open(path.c_str(),O_RDWR|O_APPEND|O_CREAT|O_EXCL,0600);
    if(fd>=0) {
      try {sync_fd(fd); Fd dir(::open(path.parent_path().c_str(),O_RDONLY)); sync_fd(dir.value);}
      catch(...) {::close(fd); throw;}
      return fd;
    }
    if(errno!=EEXIST) io_error("create log");
    return ::open(path.c_str(),O_RDWR|O_APPEND);
  }
 public:
  Journal(const std::filesystem::path& path,bool coordinator):fd_(open_log(path)),coordinator_(coordinator) {
    char buf[4];
    for(;;) {
      auto n=::read(fd_.value,buf,sizeof(buf));
      if(n<0) {if(errno==EINTR) continue; io_error("read log");}
      if(n==0) break;
      records_.append(buf,static_cast<std::size_t>(n));
      if(records_.size()>2) throw std::runtime_error("oversized log");
    }
    if(!valid(records_)) throw std::runtime_error("corrupt state log");
  }
  char state() const {
    if(poisoned_) throw std::runtime_error("I/O uncertain: reopen journal");
    return records_.empty()?'I':records_.back();
  }
  void append(char next) {
    if(state()==next) return;
    if(!valid(records_+next)) throw std::logic_error("illegal state transition");
    poisoned_=true;
    for(;;) {
      auto n=::write(fd_.value,&next,1);
      if(n==1) break;
      if(n<0 && errno==EINTR) continue;
      if(n==0) throw std::runtime_error("zero write");
      io_error("write log");
    }
    sync_fd(fd_.value);
    records_+=next; poisoned_=false;
  }
};
class Protocol {
  Journal coordinator_;
  std::array<Journal,2> participants_;
 public:
  explicit Protocol(const std::filesystem::path& dir):coordinator_(dir/"coordinator.log",true),
    participants_{{Journal(dir/"participant0.log",false),Journal(dir/"participant1.log",false)}} {}
  char participant(std::size_t i) const {return participants_.at(i).state();}
  char decision() const {return coordinator_.state();}
  bool prepare(std::size_t i,bool yes) {
    auto& p=participants_.at(i);
    if(decision()!='I') throw std::logic_error("prepare after decision");
    if(p.state()=='I') p.append(yes?'P':'A');
    return p.state()=='P';
  }
  char decide() {
    if(decision()=='I') coordinator_.append(participant(0)=='P' && participant(1)=='P'?'C':'A');
    return decision();
  }
  void recover_abort() {if(decision()=='I') coordinator_.append('A');}
  bool deliver(std::size_t i) {
    auto& p=participants_.at(i);
    if(decision()=='I') return false;
    p.append(decision()); return true;
  }
  bool blocked(std::size_t i) const {return participant(i)=='P';}
  int value(std::size_t i) const {
    static constexpr int base[2]={10,20},delta[2]={-3,3};
    auto state=participant(i); return base[i]+(state=='C'?delta[i]:0);
  }
};
}

#pragma once
#include <cstddef>
#include <deque>
#include <stdexcept>
#include <vector>
namespace tutorial {
enum class Mode { Async, Sync };
struct Entry { std::size_t sequence; int value; Mode mode; };
class Replication {
  std::vector<Entry> primary_, replica_;
  std::deque<Entry> apply_;
  std::deque<std::size_t> acks_;
  std::size_t confirmed_=0;
  bool primary_alive_=true, replica_alive_=true, link_=true;
 public:
  std::size_t write(int value, Mode mode) {
    if(!primary_alive_) throw std::runtime_error("primary unavailable");
    Entry e{primary_.size()+1,value,mode};
    primary_.push_back(e); apply_.push_back(e); return e.sequence;
  }
  bool acknowledged(std::size_t sequence) const {
    if(sequence==0 || sequence>primary_.size()) throw std::out_of_range("unknown write");
    return primary_[sequence-1].mode==Mode::Async || confirmed_>=sequence;
  }
  bool deliver_apply() {
    if(!link_ || !replica_alive_ || apply_.empty()) return false;
    auto e=apply_.front();
    if(e.sequence!=replica_.size()+1) throw std::runtime_error("out of order Apply");
    replica_.push_back(e); acks_.push_back(e.sequence); apply_.pop_front(); return true;
  }
  bool deliver_ack() {
    if(!link_ || !primary_alive_ || acks_.empty()) return false;
    auto seq=acks_.front();
    if(seq!=confirmed_+1) throw std::runtime_error("out of order Ack");
    confirmed_=seq; acks_.pop_front(); return true;
  }
  void set_link(bool up) {link_=up;}
  void set_replica_alive(bool alive) {replica_alive_=alive;}
  void fail_primary() {primary_alive_=false; apply_.clear(); acks_.clear();}
  std::size_t lag() const {return primary_.size()-replica_.size();}
  std::vector<int> read_replica() const {
    if(!replica_alive_) throw std::runtime_error("replica unavailable");
    std::vector<int> values; for(auto e:replica_) values.push_back(e.value); return values;
  }
};
}

#pragma once
#include "page.h"
#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
namespace storage {
class SlottedPage {
  Page data_{};
  std::size_t word(std::size_t at) const { return data_.at(at) | (std::size_t(data_.at(at+1))<<8); }
  void word(std::size_t at,std::size_t n) { data_.at(at)=static_cast<unsigned char>(n); data_.at(at+1)=static_cast<unsigned char>(n>>8); }
  std::size_t entry(std::size_t slot) const {
    if(slot>=slots()) throw std::out_of_range("invalid slot");
    return 8+4*slot;
  }
  void place(std::size_t e,const std::string& value) {
    auto upper=word(6)-value.size();
    std::copy(value.begin(),value.end(),data_.begin()+upper);
    word(e,upper); word(e+2,value.size()); word(6,upper);
  }
 public:
  SlottedPage() { data_[0]='S'; data_[1]=1; word(4,8); word(6,page_size); }
  explicit SlottedPage(const Page& bytes):data_(bytes) { validate(); }
  const Page& bytes() const { return data_; }
  std::size_t slots() const { return word(2); }
  std::size_t contiguous_free() const { return word(6)-word(4); }
  std::optional<std::string> get(std::size_t slot) const {
    auto e=entry(slot), n=word(e+2), off=word(e);
    if(n==0) return std::nullopt;
    return std::string(data_.begin()+off,data_.begin()+off+n);
  }
  void erase(std::size_t slot) {
    auto e=entry(slot); if(word(e+2)==0) throw std::out_of_range("deleted slot");
    word(e,0); word(e+2,0); // payload is a hole until compact()
  }
  void compact() {
    SlottedPage packed;
    packed.word(2,slots()); packed.word(4,8+4*slots());
    for(std::size_t i=0;i<slots();++i) if(auto value=get(i)) packed.place(8+4*i,*value);
    data_=packed.data_;
  }
  std::optional<std::size_t> insert(const std::string& value) {
    if(value.empty()) throw std::invalid_argument("empty records unsupported");
    if(value.size()>page_size-12) return std::nullopt;
    // ponytail: one page copy per mutation; a free-space index is unnecessary at 256 bytes.
    auto candidate=*this; candidate.compact();
    if(value.size()+4>candidate.contiguous_free()) return std::nullopt;
    auto slot=slots(); candidate.word(2,slot+1); candidate.word(4,8+4*(slot+1));
    candidate.place(8+4*slot,value); data_=candidate.data_; return slot;
  }
  bool update(std::size_t slot,const std::string& value) {
    if(value.empty()) throw std::invalid_argument("empty records unsupported");
    if(!get(slot)) throw std::out_of_range("deleted slot");
    if(value.size()>page_size-8) return false;
    auto candidate=*this; candidate.erase(slot); candidate.compact();
    if(value.size()>candidate.contiguous_free()) return false;
    candidate.place(8+4*slot,value); data_=candidate.data_; return true;
  }
  void validate() const {
    if(data_[0]!='S' || data_[1]!=1 || slots()>(page_size-8)/4 || word(4)!=8+4*slots()
       || word(6)<word(4) || word(6)>page_size) throw std::runtime_error("bad page header");
    std::array<bool,page_size> used{};
    for(std::size_t i=0;i<slots();++i) {
      auto e=8+4*i, off=word(e), n=word(e+2);
      if(n==0) { if(off!=0) throw std::runtime_error("bad tombstone"); continue; }
      if(off<word(6) || off>page_size || n>page_size-off) throw std::runtime_error("bad slot bounds");
      for(std::size_t j=off;j<off+n;++j) {
        if(used[j]) throw std::runtime_error("overlapping records");
        used[j]=true;
      }
    }
  }
};
}

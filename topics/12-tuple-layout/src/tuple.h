#pragma once
#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <stdexcept>
namespace storage {
using Bytes = std::vector<unsigned char>;
struct Tuple {
  std::optional<std::uint32_t> id;
  std::optional<std::string> name;
  bool operator==(const Tuple& t) const { return id==t.id && name==t.name; }
};
inline void put16(Bytes& b, std::size_t at, std::size_t n) {
  b.at(at)=static_cast<unsigned char>(n); b.at(at+1)=static_cast<unsigned char>(n>>8);
}
inline std::size_t get16(const Bytes& b, std::size_t at) { return b.at(at) | (std::size_t(b.at(at+1))<<8); }
// v1: version(1), null bitmap(1), total(2), uint32 id(4), offset(2), length(2), payload.
inline Bytes encode(const Tuple& t) {
  const auto n = t.name ? t.name->size() : 0;
  if(n>65535-12) throw std::length_error("tuple exceeds uint16 format");
  Bytes b(12+n,0); b[0]=1; b[1]=(!t.id ? 1 : 0) | (!t.name ? 2 : 0);
  put16(b,2,b.size());
  if(t.id) for(unsigned i=0;i<4;++i) b[4+i]=static_cast<unsigned char>(*t.id>>(8*i));
  put16(b,8,12); put16(b,10,n);
  if(t.name) std::copy(t.name->begin(),t.name->end(),b.begin()+12);
  return b;
}
inline Tuple decode(const Bytes& b) {
  if(b.size()<12 || b.size()>65535 || b[0]!=1 || (b[1]&~3)!=0 || get16(b,2)!=b.size())
    throw std::runtime_error("invalid tuple header");
  const auto off=get16(b,8), n=get16(b,10);
  if(off!=12 || n!=b.size()-12 || ((b[1]&2)!=0 && n!=0))
    throw std::runtime_error("invalid variable field bounds");
  std::uint32_t id=0;
  for(unsigned i=0;i<4;++i) id |= std::uint32_t(b[4+i])<<(8*i);
  if((b[1]&1)!=0 && id!=0) throw std::runtime_error("noncanonical null id");
  Tuple t;
  if((b[1]&1)==0) t.id=id;
  if((b[1]&2)==0) t.name=std::string(b.begin()+12,b.end());
  return t;
}
}

#include "slotted_page.h"
#include "check.h"
#include <vector>
int main() { try {
  using namespace storage;
  SlottedPage p; p.validate(); rejects([&]{p.get(0);}); rejects([&]{p.insert("");});
  auto a=*p.insert("cat"),b=*p.insert("elephant"); p.erase(a); auto free=p.contiguous_free();
  CHECK(!p.get(a)); p.compact(); CHECK(p.contiguous_free()==free+3); CHECK(*p.get(b)=="elephant");
  CHECK(p.update(b,"a longer record")); CHECK(*p.get(b)=="a longer record"); CHECK(p.update(b,"x"));
  auto bytes=p.bytes(); CHECK(!p.update(b,std::string(256,'z'))); CHECK(p.bytes()==bytes);
  CHECK(!p.insert(std::string(245,'z'))); CHECK(p.bytes()==bytes);
  rejects([&]{p.erase(a);}); rejects([&]{p.update(a,"x");});
  SlottedPage full; CHECK(full.insert(std::string(244,'x'))==0); bytes=full.bytes();
  CHECK(!full.insert("y")); CHECK(full.bytes()==bytes); CHECK(*full.get(0)==std::string(244,'x'));
  SlottedPage q; std::vector<std::optional<std::string>> oracle;
  for(int k=0;k<400;++k) {
    if(k%3==0 && !oracle.empty()) { auto i=std::size_t(k)%oracle.size(); if(oracle[i]) {q.erase(i);oracle[i].reset();} }
    else if(k%3==1 && !oracle.empty()) {auto i=std::size_t(k)%oracle.size(); if(oracle[i]) {std::string s(std::size_t(k)%31+1,'u'); if(q.update(i,s)) oracle[i]=s;} }
    else {std::string s(std::size_t(k)%17+1,'a'); if(auto i=q.insert(s)) {CHECK(*i==oracle.size());oracle.push_back(s);} }
    if(k%7==0) q.compact();
    q.validate(); SlottedPage reloaded(q.bytes());
    for(std::size_t i=0;i<oracle.size();++i) CHECK(reloaded.get(i)==oracle[i]);
  }
  bytes=p.bytes(); bytes[0]=0; rejects([&]{SlottedPage bad(bytes);});
  bytes=p.bytes(); bytes[4]=255; rejects([&]{SlottedPage bad(bytes);});
  bytes=p.bytes(); bytes[12]=1; bytes[13]=0; rejects([&]{SlottedPage bad(bytes);});
  SlottedPage overlap; overlap.insert("aaa"); overlap.insert("bbb"); bytes=overlap.bytes(); bytes[12]=bytes[8]; bytes[13]=bytes[9];
  rejects([&]{SlottedPage bad(bytes);});
  std::cout<<"slotted page checks passed\n";
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;} }

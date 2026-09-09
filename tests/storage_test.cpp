#include "tuple.h"
#include "check.h"
int main() { try {
  using namespace storage;
  for(auto id : {std::optional<std::uint32_t>{},std::optional<std::uint32_t>{0},std::optional<std::uint32_t>{UINT32_MAX}})
    for(auto name : {std::optional<std::string>{},std::optional<std::string>{""},std::optional<std::string>{std::string("a\0b",3)},std::optional<std::string>{"数据库"}}) {
      Tuple t{id,name}; CHECK(decode(encode(t))==t);
    }
  Tuple max{7,std::string(65523,'x')}; CHECK(encode(max).size()==65535); CHECK(decode(encode(max))==max);
  rejects([&]{encode({1,std::string(65524,'x')});});
  auto good=encode({42,std::string("Ada")});
  CHECK(good[4]==42); CHECK(get16(good,8)==12); CHECK(get16(good,10)==3);
  for(std::size_t n=0;n<good.size();++n) rejects([&]{decode(Bytes(good.begin(),good.begin()+n));});
  for(auto pos : {0,1,2,8,10}) { auto b=good; b[pos]=255; rejects([&]{decode(b);}); }
  auto b=good; b.push_back(0); rejects([&]{decode(b);});
  b=good; b[1]=2; rejects([&]{decode(b);});
  b=encode({std::nullopt,std::nullopt}); b[4]=1; rejects([&]{decode(b);});
  std::cout<<"tuple checks passed\n";
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; } }

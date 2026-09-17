#include "codec.h"
#include "check.h"
#include <limits>
#include <random>
using namespace tutorial;
int main() {
  const auto max = std::numeric_limits<std::uint64_t>::max();
  CHECK(bit_width(0)==0 && bit_width(1)==1 && bit_width(max)==64);
  for (const Values &v : {Values{}, Values{0}, Values{max}, Values(100,max), Values(100,0)})
    CHECK(decode(encode(v))==v);
  CHECK(encode(Values(100,0)).size()==5);
  CHECK(encode({1,7,3,5}) == Bytes({4,0,0,0,3,249,10}));
  CHECK(decode(encode({max,0,max},64))==Values({max,0,max}));
  CHECK(decode(encode({0,0},0))==Values({0,0}));
  rejects([]{encode({1},0);});
  rejects([]{encode({8},3);});
  rejects([]{encode({},65);});
  rejects([&]{encode(Values(max_items+1));});
  std::mt19937_64 rng(19445);
  for (unsigned width=0; width<=64; ++width) {
    const auto mask = width==64 ? max : (std::uint64_t(1)<<width)-1;
    for (int trial=0; trial<10; ++trial) {
      Values v;
      for (std::size_t i=0,n=rng()%100; i<n; ++i) v.push_back(rng() & mask);
      CHECK(decode(encode(v,width))==v);
    }
  }
  const auto valid = encode({1,7,3,5});
  for (std::size_t n=0; n<valid.size(); ++n)
    rejects([&]{decode(Bytes(valid.begin(),valid.begin()+static_cast<std::ptrdiff_t>(n)));});
  auto bad=valid; bad.push_back(0); rejects([&]{decode(bad);});
  bad=valid; bad[4]=65; rejects([&]{decode(bad);});
  bad=valid; bad.back() |= 128; rejects([&]{decode(bad);}); // nonzero padding
  bad=valid; bad[0]=255; bad[1]=255; bad[2]=255; bad[3]=255;
  rejects([&]{decode(bad);});
}

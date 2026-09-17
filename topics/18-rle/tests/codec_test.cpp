#include "codec.h"
#include "check.h"
#include <limits>
#include <random>
using namespace tutorial;
int main() {
  const auto max = std::numeric_limits<std::uint64_t>::max();
  for (const Values &values : {Values{}, Values{1}, Values{max}, Values(1000,max), Values{1,2,1,2}})
    CHECK(decode(encode(values)) == values);
  CHECK(encode(Values(100,7)).size() == 16);
  CHECK(encode(Values{1,2,3}).size() == 40); // expansion, not universal compression
  std::mt19937_64 rng(18445);
  for (int trial = 0; trial < 500; ++trial) {
    Values values;
    for (std::size_t i = 0, n = rng()%200; i < n; ++i)
      values.push_back(trial%2 ? rng()%4 : rng());
    CHECK(decode(encode(values)) == values);
  }
  const auto valid = encode({7,7,8});
  for (std::size_t n=0; n<valid.size(); ++n)
    rejects([&]{ decode(Bytes(valid.begin(),valid.begin()+static_cast<std::ptrdiff_t>(n))); });
  auto bad = valid; bad.push_back(0); rejects([&]{decode(bad);});
  bad = valid; bad[12]=0; rejects([&]{decode(bad);}); // zero run
  bad = valid; bad[12]=4; rejects([&]{decode(bad);}); // exceeds total
  bad = valid; bad[16]=7; rejects([&]{decode(bad);}); // adjacent equal runs
  bad = valid; bad[0]=255; bad[1]=255; bad[2]=255; bad[3]=255;
  rejects([&]{decode(bad);});
  rejects([&]{encode(Values(max_items+1));});
}

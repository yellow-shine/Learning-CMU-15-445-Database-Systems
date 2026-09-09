#include "codec.h"
#include "check.h"
#include <limits>
#include <random>
using namespace tutorial;
int main() {
  const auto low = std::numeric_limits<std::int64_t>::min();
  const auto high = std::numeric_limits<std::int64_t>::max();
  for (const auto &v : {Values{}, Values{0}, Values{low}, Values{high}, Values(100,high),
       Values(100,low), Values{low,-1}, Values{high,-1}, Values{0,low}, Values{0,high},
       Values{5,4,3,2,1}, Values{1000,1001,1003,1002,1002}})
    CHECK(decode(encode(v)) == v);
  CHECK(encode({1000,1001,1003,1002,1002}) == Bytes({5,0,0,0,208,15,2,4,1,0}));
  CHECK(encode({}).size() == 4);
  CHECK(encode(Values(100,0)).size() == 104);
  CHECK(encode({low}).size() == 14); // expansion for a single large base
  CHECK(decode(encode(Values(max_items,0))).size() == max_items);
  rejects([&]{encode(Values(max_items+1));});
  for (const auto &v : {Values{low,high}, Values{high,low}, Values{low,0}, Values{1,low}})
    rejects([&]{encode(v);});
  std::mt19937_64 rng(21445);
  std::uniform_int_distribution<std::int64_t> full(low,high);
  std::uniform_int_distribution<std::int64_t> step(-1000,1000);
  for (int trial = 0; trial < 500; ++trial) {
    const Values single{full(rng)};
    CHECK(decode(encode(single)) == single);
    Values values;
    std::int64_t current = step(rng);
    for (std::size_t i = 0, n = rng()%200; i < n; ++i) {
      current += step(rng); // bounded by 201000, cannot overflow
      values.push_back(current);
    }
    CHECK(decode(encode(values)) == values);
  }
  const auto valid = encode({1000,1001,1003});
  for (std::size_t n = 0; n < valid.size(); ++n)
    rejects([&]{decode(Bytes(valid.begin(),valid.begin()+static_cast<std::ptrdiff_t>(n)));});
  auto bad = valid; bad.push_back(0); rejects([&]{decode(bad);});
  bad = valid; bad[0]=255; bad[1]=255; bad[2]=255; bad[3]=255;
  rejects([&]{decode(bad);});
  rejects([]{decode({1,0,0,0,128,0});}); // overlong zero
  bad = encode({low}); bad.back() = 2; rejects([&]{decode(bad);}); // >64 bits
  bad.back() = 129; rejects([&]{decode(bad);}); // continuation beyond ten bytes
  // Append an otherwise valid signed value as a delta; reconstruction must reject overflow.
  for (const auto &pair : {Values{high,1}, Values{low,-1}, Values{low,low}, Values{high,high}}) {
    bad = encode({pair[0]}); bad[0] = 2;
    const auto delta = encode({pair[1]});
    bad.insert(bad.end(),delta.begin()+4,delta.end());
    rejects([&]{decode(bad);});
  }
}

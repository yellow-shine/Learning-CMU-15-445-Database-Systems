#include "codec.h"
#include "check.h"
#include <random>
using namespace tutorial;
void check(const Values &values) {
  const auto bytes = encode(values);
  CHECK(decode(bytes) == values);
  Values keys = values;
  keys.push_back("absent");
  for (const auto &key : keys) {
    std::vector<std::size_t> expected;
    for (std::size_t i = 0; i < values.size(); ++i)
      if (values[i] == key) expected.push_back(i);
    CHECK(filter_equal(bytes, key) == expected);
  }
}
int main() {
  for (const auto &values : {Values{}, Values{""}, Values{"one"}, Values(100,"repeat"),
       Values{"a","b","a"}, Values{std::string("a\0b",3), std::string(1, '\xff'), "中文"}})
    check(values);
  CHECK(encode({}) == Bytes({0,0,0,0,0,0,0,0}));
  CHECK(encode({"a","a"}) == Bytes({2,0,0,0,1,0,0,0,1,0,0,0,97,0,0,0,0,0,0,0,0}));
  CHECK(encode(Values(100,"database")).size() == 420);
  const Values largest{std::string(max_string_bytes, '\xff')};
  CHECK(decode(encode(largest)) == largest);
  CHECK(decode(encode(Values(max_items,""))).size() == max_items);
  rejects([]{encode(Values(max_items + 1));});
  rejects([]{encode({std::string(max_string_bytes + 1,'x')});});
  std::mt19937 rng(20445);
  for (int trial = 0; trial < 200; ++trial) {
    Values values;
    for (unsigned i = 0, n = rng()%50; i < n; ++i) {
      std::string value;
      for (unsigned j = 0, length = rng()%10; j < length; ++j)
        value.push_back(static_cast<char>(rng()%256));
      values.push_back(trial%2 ? std::to_string(rng()%4) : value);
    }
    check(values);
  }
  const auto valid = encode({"a","b","a"});
  for (std::size_t n = 0; n < valid.size(); ++n)
    rejects([&]{decode(Bytes(valid.begin(),valid.begin()+static_cast<std::ptrdiff_t>(n)));});
  auto bad = valid; bad.push_back(0); rejects([&]{decode(bad);});
  bad = valid; bad.back() = 255; rejects([&]{decode(bad);}); // UINT32 high ID
  rejects([&]{filter_equal(bad,"absent");});
  bad = valid; bad[17] = 'a'; rejects([&]{decode(bad);}); // duplicate dictionary
  bad = valid; bad[8] = 255; rejects([&]{decode(bad);}); // string exceeds remaining bytes
  bad = valid; bad[4] = 4; rejects([&]{decode(bad);}); // d > n
  bad = valid; bad[0]=255; bad[1]=255; bad[2]=255; bad[3]=255;
  rejects([&]{decode(bad);});
  // Valid dictionary size, but repeated ID would expand beyond the byte budget.
  bad = encode({std::string(max_string_bytes,'x')});
  bad[0] = 2; put(bad,0,4); rejects([&]{decode(bad);});
}

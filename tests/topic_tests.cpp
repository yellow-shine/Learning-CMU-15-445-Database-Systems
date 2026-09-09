#include "query.h"
#include "check.h"
#include <map>
#include <random>
#include <set>
int main() {
  CHECK(live_ids({}).empty() && region_counts({}).empty());
  CHECK(live_ids({{1,"x",true}}).empty());
  std::mt19937 rng(445);
  for (int round = 0; round < 100; ++round) {
    std::vector<Row> rows;
    std::set<int> expected;
    std::map<std::string,std::size_t> counts;
    for (int i = 0; i < round; ++i) {
      Row r{static_cast<int>(rng()%20), std::to_string(rng()%5), rng()%3 == 0};
      rows.push_back(r);
      if (!r.deleted) { expected.insert(r.id); ++counts[r.region]; }
    }
    auto ids = live_ids(rows);
    CHECK(ids == std::vector<int>(expected.begin(), expected.end()));
    auto groups = region_counts(rows);
    CHECK(groups.size() == counts.size());
    for (const auto &entry : groups) CHECK(counts.at(entry.first) == entry.second);
    for (int id = -1; id < 21; ++id)
      CHECK(std::binary_search(ids.begin(), ids.end(), id) == (expected.count(id) != 0));
    CHECK(rows.size() == static_cast<std::size_t>(round));
  }
  std::vector<Row> rows{{7,"x",false}};
  auto index = std::size_t{0};
  rows.reserve(rows.capacity()+1); CHECK(rows.at(index).id == 7);
  rows.insert(rows.begin(), {9,"y",false});
  // Index stability is NOT identity stability after insertion before it.
  CHECK(rows.at(index).id == 9 && rows.at(index+1).id == 7);
  auto next = rows.erase(rows.begin()); CHECK(next == rows.begin() && next->id == 7);
  std::unordered_map<int,int> hash{{7,42}};
  int &value = hash.at(7);
  hash.rehash(hash.bucket_count()+10);
  CHECK(value == 42 && hash.find(7)->second == 42); // references survive rehash, iterators do not.
}

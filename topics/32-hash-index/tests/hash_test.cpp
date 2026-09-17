#include "hash_table.h"
#include <climits>
#include <iostream>
#include <map>
#include <random>
#include <set>
#define CHECK(x) do { if (!(x)) { std::cerr << "line " << __LINE__ << ": " #x "\n"; return 1; } } while (false)
int main() {
  bool rejected = false;
  try { HashIndex bad(0); } catch (const std::invalid_argument&) { rejected = true; }
  CHECK(rejected);
  HashIndex t(5);
  CHECK(t.lookup(7).empty() && !t.erase(7, {1, 0}));
  CHECK(t.insert(7, {1, 0}) && t.insert(7, {1, 1}) && t.insert(7, {2, 0}));
  CHECK(!t.insert(7, {1, 0}) && t.size() == 3 && t.key_count() == 1);
  CHECK(t.insert(12, {1, 0})); // Same RID under another key is not forbidden by this index.
  CHECK(!t.erase(7, {9, 0}) && t.size() == 4);
  CHECK(t.erase(7, {1, 0}) && t.lookup(7).size() == 2 && t.lookup(12).size() == 1);
  auto copy = t.lookup(7); copy.clear(); CHECK(t.lookup(7).size() == 2);
  CHECK(t.erase(7, {1, 1}) && t.erase(7, {2, 0}) && t.lookup(7).empty());
  CHECK(t.key_count() == 1 && t.size() == 1 && t.valid());
  CHECK(t.insert(7, {UINT32_MAX, UINT32_MAX}) && t.erase(7, {UINT32_MAX, UINT32_MAX}));
  CHECK(t.insert(INT_MIN, {0, 0}) && t.insert(INT_MAX, {0, 0}) && t.valid());
  HashIndex h(7);
  std::map<int, std::set<RID>> oracle;
  std::mt19937 rng(3201);
  for (int n = 0; n < 8000; ++n) {
    int key = static_cast<int>(rng() % 41) - 20;
    RID rid{static_cast<std::uint32_t>(rng() % 4), static_cast<std::uint32_t>(rng() % 8)};
    if (rng() % 2) {
      bool expected = oracle[key].insert(rid).second;
      CHECK(h.insert(key, rid) == expected);
    } else {
      auto it = oracle.find(key);
      bool expected = it != oracle.end() && it->second.erase(rid) != 0;
      CHECK(h.erase(key, rid) == expected);
      if (it != oracle.end() && it->second.empty()) oracle.erase(it);
    }
    std::size_t total = 0;
    for (int k = -20; k <= 20; ++k) {
      auto actual = h.lookup(k);
      std::sort(actual.begin(), actual.end());
      const auto it = oracle.find(k);
      std::vector<RID> expected;
      if (it != oracle.end()) expected.assign(it->second.begin(), it->second.end());
      CHECK(actual == expected);
      total += expected.size();
    }
    CHECK(h.size() == total && h.key_count() == oracle.size() && h.valid());
  }
  for (const auto& e : oracle) for (auto rid : e.second) CHECK(h.erase(e.first, rid));
  CHECK(h.size() == 0 && h.key_count() == 0 && h.valid());
}

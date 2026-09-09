#include "hash_table.h"
#include <climits>
#include <iostream>
#include <map>
#include <random>
#define CHECK(x) do { if (!(x)) { std::cerr << "line " << __LINE__ << ": " #x "\n"; return 1; } } while (false)
int main() {
  bool rejected = false;
  try { LinearProbing bad(0); } catch (const std::invalid_argument&) { rejected = true; }
  CHECK(rejected);
  LinearProbing t(5);
  CHECK(!t.get(0) && !t.erase(0));
  for (int k : {4, 9, 14, 19, 24}) CHECK(t.put(k, k));
  CHECK(!t.put(29, 29));
  CHECK(t.erase(9)); CHECK(t.get(24) == 24);
  CHECK(t.put(24, 42)); CHECK(t.size() == 4);
  CHECK(t.put(29, 29)); CHECK(t.size() == 5);
  for (int k : {4, 14, 19, 24, 29}) CHECK(t.erase(k));
  CHECK(t.put(INT_MIN, 1) && t.put(INT_MAX, 2));
  LinearProbing h(31);
  std::map<int, int> oracle;
  std::mt19937 rng(2801);
  for (int n = 0; n < 12000; ++n) {
    int k = static_cast<int>(rng() % 101) - 50;
    int value = static_cast<int>(rng() % 10000);
    if (rng() % 2) {
      bool expected = oracle.count(k) || oracle.size() < 31;
      CHECK(h.put(k, value) == expected);
      if (expected) oracle[k] = value;
    } else { CHECK(h.erase(k) == (oracle.erase(k) != 0)); }
    CHECK(h.size() == oracle.size());
    for (int key = -50; key <= 50; ++key) {
      auto it = oracle.find(key);
      CHECK(h.get(key) == (it == oracle.end() ? std::optional<int>{} : it->second));
    }
  }
}

#include "hash_table.h"
#include <climits>
#include <iostream>
#include <map>
#include <random>
#define CHECK(x) do { if (!(x)) { std::cerr << "line " << __LINE__ << ": " #x "\n"; return 1; } } while (false)
int main() {
  for (int mode = 0; mode < 3; ++mode) {
    bool rejected = false;
    try { CuckooHash bad(mode == 0 ? 0 : 2, mode == 1 ? 1 : 100, mode == 2 ? 17 : 2); }
    catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
  }
  CuckooHash tiny(1, 1, 2);
  CHECK(!tiny.get(1) && !tiny.erase(1));
  CHECK(tiny.put(1, 10) && tiny.put(2, 20));
  CHECK(tiny.last_stats().kicks == 1);
  CHECK(!tiny.put(3, 30));
  CHECK(tiny.last_stats().cycles > 0 && tiny.last_stats().rebuilds == 2);
  CHECK(tiny.last_stats().kicks <= 3 * 3 * 8);
  CHECK(tiny.get(1) == 10 && tiny.get(2) == 20 && !tiny.get(3));
  CHECK(tiny.size() == 2 && tiny.valid());
  CHECK(tiny.put(1, 11) && tiny.size() == 2 && tiny.get(1) == 11);
  CHECK(tiny.erase(1) && tiny.put(INT_MIN, 7) && tiny.get(INT_MIN) == 7);
  CuckooHash no_retry(1, 1, 0);
  CHECK(no_retry.put(1, 1) && no_retry.put(2, 2) && !no_retry.put(3, 3));
  CHECK(no_retry.get(1) == 1 && no_retry.get(2) == 2 && no_retry.last_stats().rebuilds == 0);
  CuckooHash growing(1);
  CHECK(growing.put(INT_MIN, 1) && growing.put(INT_MAX, 2));
  std::map<int, int> oracle{{INT_MIN, 1}, {INT_MAX, 2}};
  std::mt19937 rng(3001);
  std::size_t rebuilds = 0;
  for (int n = 0; n < 5000; ++n) {
    const int k = static_cast<int>(rng() % 301) - 150;
    if (rng() % 3) {
      int value = static_cast<int>(rng() % 10000);
      CHECK(growing.put(k, value));
      oracle[k] = value;
      rebuilds += growing.last_stats().rebuilds;
    } else CHECK(growing.erase(k) == (oracle.erase(k) != 0));
    CHECK(growing.size() == oracle.size() && growing.valid());
    for (int key = -150; key <= 150; ++key) {
      auto it = oracle.find(key);
      CHECK(growing.get(key) == (it == oracle.end() ? std::optional<int>{} : it->second));
    }
    CHECK(growing.get(INT_MIN) == 1 && growing.get(INT_MAX) == 2);
  }
  CHECK(rebuilds > 0 && growing.capacity() > 1);
}

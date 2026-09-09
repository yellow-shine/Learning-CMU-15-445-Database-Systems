#include "hash_table.h"
#include <climits>
#include <iostream>
#include <map>
#include <random>
#define CHECK(x) do { if (!(x)) { std::cerr << "line " << __LINE__ << ": " #x "\n"; return 1; } } while (false)
int main() {
  for (int mode = 0; mode < 2; ++mode) {
    bool rejected = false;
    try { ExtendibleHash bad(mode ? 2 : 0, mode ? 17 : 16); }
    catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
  }
  ExtendibleHash t(2);
  CHECK(t.global_depth() == 0 && t.directory_size() == 1 && t.valid());
  CHECK(!t.get(0) && !t.erase(0));
  for (int k : {0, 2, 1, 4}) CHECK(t.put(k, 10 + k));
  CHECK(t.global_depth() == 2 && t.directory_size() == 4);
  CHECK(t.local_depth(0) == 2 && t.local_depth(2) == 2);
  CHECK(t.local_depth(1) == 1 && t.local_depth(3) == 1 && t.same_bucket(1, 3));
  CHECK(!t.same_bucket(0, 2) && t.valid());
  for (int k : {0, 2, 1, 4}) CHECK(t.get(k) == 10 + k);
  // Splitting an aliased bucket with local < global must not double directory.
  CHECK(t.put(3, 13) && t.put(5, 15));
  CHECK(t.global_depth() == 2 && !t.same_bucket(1, 3) && t.valid());
  ExtendibleHash capped(1, 2);
  CHECK(capped.put(0, 10) && !capped.put(4, 40));
  CHECK(capped.global_depth() == 2 && capped.get(0) == 10 && !capped.get(4) && capped.valid());
  CHECK(capped.put(0, 11) && capped.size() == 1);
  CHECK(capped.erase(0) && capped.put(4, 40));
  ExtendibleHash depth_zero(1, 0);
  CHECK(depth_zero.put(1, 1) && !depth_zero.put(2, 2) && depth_zero.valid());
  ExtendibleHash h(3);
  std::map<int, int> oracle;
  std::mt19937 rng(3101);
  for (int n = 0; n < 7000; ++n) {
    int k = static_cast<int>(rng() % 401) - 200;
    if (rng() % 2) { int v = static_cast<int>(rng() % 10000); CHECK(h.put(k, v)); oracle[k] = v; }
    else CHECK(h.erase(k) == (oracle.erase(k) != 0));
    CHECK(h.size() == oracle.size() && h.valid());
    for (int key = -200; key <= 200; ++key) {
      auto it = oracle.find(key);
      CHECK(h.get(key) == (it == oracle.end() ? std::optional<int>{} : it->second));
    }
  }
  CHECK(h.put(INT_MIN, 1) && h.put(INT_MAX, 2));
  CHECK(h.get(INT_MIN) == 1 && h.get(INT_MAX) == 2 && h.valid());
}

#include "lru_k.h"
#include "check.h"
int main() {
  rejects([] { LRUK invalid(1, 0); });
  LRUK empty(0, 2); CHECK(!empty.evict()); rejects([&] { empty.access(0); });
  LRUK lru(3, 2); CHECK(!lru.evict()); rejects([&] { lru.set_evictable(0, true); });
  for (auto id : {0, 1, 0, 2, 1}) { lru.access(id); lru.set_evictable(id, true); }
  CHECK(lru.evict() == 2); CHECK(lru.evict() == 0); CHECK(lru.evict() == 1); CHECK(!lru.evict());
  LRUK cold(3, 3);
  for (auto id : {0, 1, 0, 2}) { cold.access(id); cold.set_evictable(id, true); }
  CHECK(cold.evict() == 0); // Infinite-distance tie uses first, NOT latest access.
  cold.set_evictable(1, false); CHECK(cold.evict() == 2); CHECK(!cold.evict());
  cold.access(1); CHECK(!cold.evict()); cold.set_evictable(1, true); CHECK(cold.evict() == 1);
  LRUK one(1, 2); one.access(0); CHECK(!one.evict());
  one.set_evictable(0, true); one.access(0); CHECK(one.evict() == 0); CHECK(!one.evict());
  one.access(0); CHECK(!one.evict()); rejects([&] { one.access(1); });
  LRUK rolling(2, 2);
  for (auto id : {0, 0, 1, 1, 0, 0}) { rolling.access(id); rolling.set_evictable(id, true); }
  CHECK(rolling.evict() == 1); // Old 0 timestamps must have been discarded.
  rolling.access(1); rolling.set_evictable(1, true); CHECK(rolling.evict() == 1); // Fresh cold history.
  LRUK k1(3, 1);
  for (auto id : {0, 1, 2, 0}) { k1.access(id); k1.set_evictable(id, true); }
  CHECK(k1.evict() == 1); CHECK(k1.evict() == 2); CHECK(k1.evict() == 0);
}

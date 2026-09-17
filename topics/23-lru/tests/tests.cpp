#include "lru.h"
#include "check.h"
int main() {
  LRU empty(0); CHECK(!empty.evict()); rejects([&] { empty.access(0); });
  LRU lru(3); CHECK(!lru.evict());
  rejects([&] { lru.set_evictable(0, true); });
  for (auto id : {0, 1, 2, 0}) { lru.access(id); lru.set_evictable(id, true); }
  CHECK(lru.evict() == 1);
  lru.set_evictable(2, false); CHECK(lru.evict() == 0); CHECK(!lru.evict());
  lru.access(2); CHECK(!lru.evict()); // Access does not unpin a frame.
  lru.set_evictable(2, true); CHECK(lru.evict() == 2); CHECK(!lru.evict());
  LRU one(1); one.access(0); CHECK(!one.evict()); one.set_evictable(0, true);
  one.access(0); CHECK(one.evict() == 0); CHECK(!one.evict());
  one.access(0); CHECK(!one.evict()); // Reused frame resets protection.
  rejects([&] { one.access(1); }); rejects([&] { one.set_evictable(1, true); });
  // Independent recency order oracle for a repeated deterministic trace.
  LRU tested(4); std::vector<std::size_t> order;
  for (std::size_t t = 0; t < 100; ++t) {
    std::size_t id = (t * t + t / 3) % 4;
    for (auto it = order.begin(); it != order.end(); ++it) if (*it == id) { order.erase(it); break; }
    order.push_back(id); tested.access(id); tested.set_evictable(id, true);
  }
  for (auto id : order) CHECK(tested.evict() == id);
  CHECK(!tested.evict());
}

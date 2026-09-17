#include "clock.h"
#include "check.h"
int main() {
  Clock empty(0); CHECK(!empty.evict()); rejects([&] { empty.access(0); });
  Clock clock(3); CHECK(!clock.evict()); rejects([&] { clock.set_evictable(0, true); });
  for (auto id : {0, 1, 2, 0}) { clock.access(id); clock.set_evictable(id, true); }
  CHECK(clock.evict() == 0); clock.access(1); CHECK(clock.evict() == 2);
  clock.set_evictable(1, false); CHECK(!clock.evict());
  clock.set_evictable(1, true); CHECK(clock.evict() == 1); CHECK(!clock.evict());
  Clock one(1); one.access(0); CHECK(!one.evict());
  one.set_evictable(0, true); CHECK(one.evict() == 0); CHECK(!one.evict());
  one.access(0); CHECK(!one.evict()); rejects([&] { one.access(1); });
  // Pinned entries keep their reference bits while the hand passes them.
  Clock protected_bits(3);
  for (auto id : {0, 1, 2}) protected_bits.access(id);
  protected_bits.set_evictable(1, true); CHECK(protected_bits.evict() == 1);
  protected_bits.set_evictable(0, true); protected_bits.set_evictable(2, true);
  CHECK(protected_bits.evict() == 2); CHECK(protected_bits.evict() == 0);
  // Repeated reuse does not strand the hand on absent slots.
  for (int i = 0; i < 100; ++i) {
    clock.access(2); clock.set_evictable(2, true); CHECK(clock.evict() == 2); CHECK(!clock.evict());
  }
}

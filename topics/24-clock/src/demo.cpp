#include "clock.h"
#include <iostream>
int main() {
  Clock clock(3);
  for (auto id : {0, 1, 2, 0}) { clock.access(id); clock.set_evictable(id, true); }
  std::cout << "victim: " << *clock.evict() << '\n';
  clock.access(1);
  std::cout << "after access(1): " << *clock.evict() << '\n';
}

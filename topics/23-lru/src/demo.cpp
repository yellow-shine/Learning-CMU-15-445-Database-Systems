#include "lru.h"
#include <iostream>
int main() {
  LRU lru(3);
  for (auto id : {0, 1, 2, 0}) { lru.access(id); lru.set_evictable(id, true); }
  std::cout << "victims:";
  while (auto id = lru.evict()) std::cout << ' ' << *id;
  std::cout << '\n';
}

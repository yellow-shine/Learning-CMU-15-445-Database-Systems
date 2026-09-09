#include "lru_k.h"
#include <iostream>
int main() {
  LRUK lru(3, 2);
  for (auto id : {0, 1, 0, 2, 1}) { lru.access(id); lru.set_evictable(id, true); }
  std::cout << "K=2 victims:";
  while (auto id = lru.evict()) std::cout << ' ' << *id;
  std::cout << '\n';
}

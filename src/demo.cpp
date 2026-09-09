#include "hash_table.h"
#include <iostream>
void show(const RobinHood& t) {
  for (std::size_t i = 0; i < t.slots().size(); ++i)
    if (t.slots()[i]) std::cout << "slot=" << i << " key=" << t.slots()[i]->key
                              << " distance=" << t.slots()[i]->distance << '\n';
}
int main() {
  RobinHood t(5);
  for (int k : {0, 1, 5}) t.put(k, k * 10);
  show(t);
  t.erase(0);
  std::cout << "after erase(0)\n";
  show(t);
}

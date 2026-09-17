#include "hash_table.h"
#include <iostream>
int main() {
  CuckooHash bounded(1, 1, 2);
  bounded.put(1, 10); bounded.put(2, 20);
  std::cout << "second insertion kicks=" << bounded.last_stats().kicks << '\n';
  std::cout << "third insertion accepted=" << bounded.put(3, 30) << '\n';
  std::cout << "rebuilds=" << bounded.last_stats().rebuilds
            << " preserved=" << bounded.get(1).value() << ',' << bounded.get(2).value() << '\n';
  CuckooHash growing(1);
  for (int k = 1; k <= 20; ++k) if (!growing.put(k, k * 10)) return 1;
  std::cout << "growing size=" << growing.size() << " key20=" << growing.get(20).value() << '\n';
}

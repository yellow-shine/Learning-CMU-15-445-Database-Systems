#include "hash_table.h"
#include <iostream>
int main() {
  ExtendibleHash h(2);
  for (int k : {0, 2, 1, 4}) h.put(k, k * 10);
  std::cout << "global=" << h.global_depth() << " directory=" << h.directory_size() << '\n';
  for (std::size_t i = 0; i < h.directory_size(); ++i)
    std::cout << "entry=" << i << " local=" << h.local_depth(i) << '\n';
  std::cout << "alias(1,3)=" << h.same_bucket(1, 3) << " key4=" << h.get(4).value() << '\n';
}

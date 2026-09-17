#include "hash_table.h"
#include <iostream>
void show(const HashIndex& index, int key) {
  std::cout << "key=" << key;
  for (auto rid : index.lookup(key)) std::cout << " (" << rid.page << ',' << rid.slot << ')';
  std::cout << '\n';
}
int main() {
  HashIndex index(5);
  index.insert(7, {1, 0}); index.insert(7, {1, 1}); index.insert(12, {2, 0});
  show(index, 7);
  index.erase(7, {1, 0});
  show(index, 7); show(index, 12);
  std::cout << "pairs=" << index.size() << " keys=" << index.key_count() << '\n';
}

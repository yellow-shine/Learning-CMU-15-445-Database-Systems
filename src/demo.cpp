#include "hash_table.h"
#include <iostream>
int main() {
  LinearProbing table(5);
  for (int k : {1, 6, 11}) table.put(k, k * 10);
  table.erase(6);
  std::cout << "after erase(6): 11=" << table.get(11).value() << '\n';
  table.put(11, 111);
  table.put(16, 160);
  std::cout << "11=" << table.get(11).value() << " size=" << table.size() << '\n';
}

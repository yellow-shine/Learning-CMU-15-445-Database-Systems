#include "query.h"
#include <iostream>
int main() {
  std::vector<Row> rows{{3,"east",false},{1,"west",false},{3,"east",false},{2,"west",true}};
  auto ids = live_ids(rows);
  std::cout << "distinct live ids:";
  for (int id : ids) std::cout << ' ' << id;
  std::cout << "\ncontains 3: " << std::binary_search(ids.begin(), ids.end(), 3) << '\n';
  for (const auto &entry : region_counts(rows)) std::cout << entry.first << ": " << entry.second << '\n';
  // Store an index, not an iterator across a possible reallocation.
  const std::size_t first = 0;
  rows.reserve(rows.capacity() + 1);
  std::cout << "reacquired first id: " << rows.at(first).id << '\n';
}

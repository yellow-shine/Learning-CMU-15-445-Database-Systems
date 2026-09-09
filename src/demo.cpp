#include "table.h"
#include <iostream>
#include <string>
struct Sale { std::string region; int cents; };
int main() {
  Table<Sale> sales;
  sales.insert({"east", 120}); sales.insert({"west", 80}); sales.insert({"east", 120});
  auto east = sales.select([](const Sale &s) { return s.region == "east"; });
  std::cout << "east rows: " << east.size() << '\n';
  std::cout << "total cents: " << sales.sum(0LL, &Sale::cents) << '\n';
  Table<int> ids; ids.insert(7); ids.insert(9);
  std::cout << "id sum: " << ids.sum(0, [](int id) { return id; }) << '\n';
}

#include "table.h"
#include "check.h"
#include <string>
struct Sale { std::string region; int cents; };
int main() {
  Table<Sale> sales;
  CHECK(sales.size() == 0 && sales.sum(5LL, &Sale::cents) == 5);
  CHECK(sales.select([](const Sale &) { return true; }).empty());
  sales.insert({"east",120}); sales.insert({"west",80}); sales.insert({"east",120});
  const auto &view = sales;
  auto selected = view.select([](const Sale &s) { return s.region == "east"; });
  CHECK(selected.size() == 2 && selected[0].cents == 120 && selected[1].cents == 120);
  selected[0].cents = 0; CHECK(view.sum(0LL, &Sale::cents) == 320);
  CHECK(view.select([](const Sale &) { return false; }).empty());
  int calls = 0;
  CHECK(view.select([&](const Sale &) { ++calls; return true; }).size() == 3 && calls == 3);
  bool threw = false;
  try { view.select([](const Sale &) -> bool { throw std::runtime_error("predicate"); }); }
  catch (const std::runtime_error &) { threw = true; }
  CHECK(threw && view.size() == 3 && view.sum(0LL, &Sale::cents) == 320);
  Table<int> ids; ids.insert(7); ids.insert(9); ids.insert(7);
  CHECK(ids.select([](int id) { return id == 7; }) == std::vector<int>({7,7}));
  CHECK(ids.sum(0, [](int id) { return id; }) == 23);
}

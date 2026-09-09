#include "sql.h"
#include <iostream>
using namespace tutorial;
int main() {
  Catalog catalog;
  catalog.create("people", Schema({{"id", Type::Integer}, {"team", Type::Text, true}}),
                 {{std::int64_t{1}, std::string{"DB"}}, {std::int64_t{2}, std::string{"DB"}}, {std::int64_t{3}, std::monostate{}}});
  auto logical = bind(Parser("SELECT team FROM people WHERE team = 'DB';").parse(), catalog);
  auto physical = lower(logical);
  std::cout << "logical: " << explain(logical) << "\nphysical: " << explain(*physical) << '\n';
  auto result = execute(*physical);
  for (const auto &row : result) std::cout << display(row[0]) << '\n';
  std::cout << "rows=" << result.size() << '\n';
  try { bind(Parser("SELECT missing FROM people").parse(), catalog); }
  catch (const std::invalid_argument &e) { std::cout << "rejected: " << e.what() << '\n'; }
}

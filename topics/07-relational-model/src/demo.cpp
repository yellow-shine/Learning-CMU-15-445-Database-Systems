#include "relation.h"
#include <iostream>
using namespace tutorial;
int main() {
  Relation people(Schema({{"id", Type::Integer}, {"name", Type::Text, true}}));
  people.insert({std::int64_t{1}, std::string{"Ada"}});
  people.insert({std::int64_t{1}, std::string{"Ada"}});
  people.insert({std::int64_t{2}, std::monostate{}});
  for (const auto &row : people.rows()) std::cout << display(row[0]) << ' ' << display(row[1]) << '\n';
  try { people.insert({std::string{"wrong"}, std::string{"Bob"}}); }
  catch (const std::invalid_argument &e) { std::cout << "rejected: " << e.what() << '\n'; }
  std::cout << "rows=" << people.rows().size() << '\n';
}

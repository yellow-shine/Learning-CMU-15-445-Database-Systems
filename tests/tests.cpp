#include "relation.h"
#include "check.h"
#include <iostream>
using namespace tutorial;
int main() {
  try {
    Schema schema({{"id", Type::Integer}, {"name", Type::Text, true}});
    Relation people(schema);
    check(people.insert({std::int64_t{1}, std::string{"Ada"}}), "first insert");
    check(!people.insert({std::int64_t{1}, std::string{"Ada"}}), "set duplicate");
    check(people.insert({std::int64_t{2}, std::monostate{}}), "nullable");
    rejects([&] { people.insert({std::int64_t{3}}); });
    rejects([&] { people.insert({std::string{"3"}, std::string{"bad"}}); });
    rejects([&] { people.insert({std::int64_t{3}, std::int64_t{4}}); });
    rejects([&] { people.insert({std::monostate{}, std::string{"bad"}}); });
    check(people.rows().size() == 2, "failed inserts are atomic");
    rejects([] { Schema bad({{"x", Type::Integer}, {"x", Type::Text}}); });
    rejects([] { Schema bad({{"", Type::Text}}); });
    rejects([&] { schema.index("missing"); });
    check(schema.index("name") == 1, "name binding");
    Relation zero(Schema({}));
    check(zero.rows().empty(), "empty relation");
    check(zero.insert({}) && !zero.insert({}), "zero degree tuple");
    check(display(std::monostate{}) == "NULL", "NULL display");
    std::cout << "relational-model tests passed\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}

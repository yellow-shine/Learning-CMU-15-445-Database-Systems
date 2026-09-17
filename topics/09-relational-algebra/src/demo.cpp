#include "algebra.h"
#include <iostream>
using namespace tutorial;
int main() {
  Relation people(Schema({{"id", Type::Integer}, {"team", Type::Text}}));
  people.insert({std::int64_t{1}, std::string{"DB"}});
  people.insert({std::int64_t{2}, std::string{"DB"}});
  people.insert({std::int64_t{3}, std::string{"AI"}});
  auto db = select(people, [](const Tuple &r) { return r[1] == Value{std::string{"DB"}}; });
  std::cout << "selected=" << db.rows().size() << " projected=" << project(db, {"team"}).rows().size() << '\n';
  std::cout << "union=" << combine(people, db, SetOperation::Union).rows().size()
            << " intersection=" << combine(people, db, SetOperation::Intersection).rows().size()
            << " difference=" << combine(people, db, SetOperation::Difference).rows().size() << '\n';
  std::cout << "product=" << product(people, db).rows().size() << " join=" << join(people, db, "team", "team").rows().size() << '\n';
}

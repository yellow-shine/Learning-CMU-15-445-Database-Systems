#include "sql.h"
#include "check.h"
#include <iostream>
#include <limits>
using namespace tutorial;
Value n(std::int64_t i) { return i; }
int main() {
  try {
    Catalog catalog;
    catalog.create("People", Schema({{"ID", Type::Integer}, {"team", Type::Text, true}}),
                   {{n(1), std::string{"DB"}}, {n(2), std::string{"DB"}}, {n(2), std::string{"DB"}},
                    {n(3), std::monostate{}}, {n(-4), std::string{"O'Reilly"}}});
    catalog.create("teams", Schema({{"id", Type::Integer}, {"label", Type::Text}}),
                   {{n(2), std::string{"systems"}}, {n(2), std::string{"systems"}}});
    catalog.create("empty", Schema({{"id", Type::Integer}}), {});
    auto run = [&](const std::string &sql) { return execute(*lower(bind(Parser(sql).parse(), catalog))); };
    check(run("sElEcT team FrOm PEOPLE wHeRe id = 2;") == std::vector<Tuple>{{std::string{"DB"}}, {std::string{"DB"}}}, "case fold and bag duplicates");
    check(run("SELECT * FROM people").size() == 5, "star and no filter");
    check(run("SELECT id,id FROM people WHERE id=1") == std::vector<Tuple>{{n(1), n(1)}}, "repeated output columns");
    check(run("SELECT id FROM people WHERE team='O''Reilly'") == std::vector<Tuple>{{n(-4)}}, "escaped quote");
    check(run("SELECT team FROM people WHERE id=-4") == std::vector<Tuple>{{std::string{"O'Reilly"}}}, "negative integer");
    check(run("SELECT id FROM people WHERE team=NULL").empty(), "NULL equality is UNKNOWN");
    check(run("SELECT id FROM people WHERE team=team").size() == 4, "NULL column equality is UNKNOWN");
    check(run("SELECT id FROM people WHERE id=2 AND team='DB'").size() == 2, "AND retains duplicates");
    check(run("SELECT id FROM people WHERE id=2 AND team='AI'").empty(), "AND false");
    auto sql = "SELECT people.id, teams.label FROM people, teams WHERE people.id=teams.id";
    check(run(sql) == std::vector<Tuple>(4, Tuple{n(2), std::string{"systems"}}), "many-to-many bag join");
    auto logical = bind(Parser(sql).parse(), catalog);
    check(logical.sources.size() == 2 && logical.columns == std::vector<std::size_t>{0, 3}, "binding global column offsets");
    check(logical.output[1].name == "teams.label", "output metadata");
    check(explain(logical) == "Project[2](Select[1](Product[people,teams]))", "logical explanation");
    auto physical = lower(logical);
    check(explain(*physical) == "Gather(Filter(NestedLoopProduct(SeqScan,SeqScan)))", "physical lowering");
    check(execute(*physical) == execute(*physical), "repeatable execution");
    check(run("SELECT * FROM people, teams").size() == 10, "unfiltered product");
    check(run("SELECT * FROM empty").empty() && run("SELECT * FROM people,empty").empty(), "empty source");
    for (const auto *bad : {
        "", "SELECT", "SELECT FROM people", "SELECT id, FROM people", "SELECT *,id FROM people",
        "SELECT id people", "SELECT id FROM", "SELECT id FROM people,", "SELECT id FROM people WHERE",
        "SELECT id FROM people WHERE id=", "SELECT id FROM people WHERE id=1 AND",
        "SELECT id FROM people WHERE id > 1", "SELECT id FROM people WHERE id = 1 OR id=2",
        "SELECT DISTINCT id FROM people", "SELECT id AS x FROM people", "SELECT id FROM people p",
        "SELECT id FROM people ORDER BY id", "SELECT id FROM people; SELECT id FROM people",
        "SELECT id FROM people;;", "SELECT id FROM people -- comment", "SELECT id FROM people WHERE team='oops",
        "SELECT id FROM people WHERE id=-", "SELECT id FROM people WHERE id=1and id=2",
        "SELECT id FROM people WHERE id=9223372036854775808",
        "SELECT id FROM people WHERE id=-9223372036854775809", "SELECT id FROM people WHERE id=1.0",
        "SELECT id FROM missing", "SELECT missing FROM people", "SELECT teams.id FROM people",
        "SELECT id FROM people,teams", "SELECT people.id FROM people,teams WHERE id=2",
        "SELECT people.id FROM people,teams WHERE people.id=id", "SELECT * FROM people,people",
        "SELECT id FROM people WHERE id='2'", "SELECT id FROM people WHERE id=team",
        "SELECT id FROM empty WHERE missing=1", "SELECT * FROM empty WHERE id='bad'",
        "SELECT id FROM people WHERE id=';'", "SELECT id FROM people WHERE 'id'=1",
        "SELECT id FROM people WHERE id=1 'and'", "SELECT id FROM people WHERE id=+1"})
      rejects([&] { run(bad); });
    catalog.create("limits", Schema({{"v", Type::Integer}}),
                   {{n(std::numeric_limits<std::int64_t>::min())}, {n(std::numeric_limits<std::int64_t>::max())}});
    check(run("SELECT v FROM limits WHERE v=-9223372036854775808").size() == 1, "int64 min");
    check(run("SELECT v FROM limits WHERE v=9223372036854775807").size() == 1, "int64 max");
    catalog.create("strings", Schema({{"v", Type::Text}}), {{std::string{}}, {std::string{"and"}}, {std::string{";"}}});
    check(run("SELECT v FROM strings WHERE v=''") == std::vector<Tuple>{{std::string{}}}, "empty literal");
    check(run("SELECT v FROM strings WHERE v='and'").size() == 1, "keyword literal");
    check(run("SELECT v FROM strings WHERE v=';'").size() == 1, "symbol literal");
    rejects([&] { catalog.create("people", Schema({}), {}); });
    rejects([&] { catalog.create("bad", Schema({{"x", Type::Integer}}), {{std::string{"1"}}}); });
    rejects([&] { catalog.create("bad", Schema({{"x", Type::Integer}}), {{std::monostate{}}}); });
    rejects([&] { catalog.create("bad", Schema({{"x", Type::Integer}}), {{n(1), n(2)}}); });
    rejects([&] { catalog.create("bad", Schema({{"x", Type::Integer}, {"X", Type::Text}}), {}); });
    rejects([&] { catalog.create("bad.name", Schema({}), {}); });
    rejects([&] { catalog.create("select", Schema({}), {}); });
    rejects([&] { catalog.table("bad"); });
    check(run("SELECT * FROM people").size() == 5, "failed queries and DDL do not mutate input");
    auto detached = [] {
      Catalog local; local.create("t", Schema({{"v", Type::Integer}}), {{n(9)}});
      return lower(bind(Parser("SELECT v FROM t").parse(), local));
    }();
    check(execute(*detached) == std::vector<Tuple>{{n(9)}}, "owned physical snapshot");
    std::cout << "sql-planning tests passed\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}

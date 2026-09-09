#include "external_sort.h"
#include <iostream>
#include <limits>
#include <sstream>
#include <iterator>
using namespace tutorial;
void check(bool ok) { if (!ok) throw std::runtime_error("check failed"); }
template<class F> void rejects(F fn) {
  bool caught = false;
  try { fn(); } catch (const std::exception&) { caught = true; }
  check(caught);
}
void verify(const std::vector<Value>& values, std::size_t budget, std::size_t fan) {
  Workspace input;
  auto file = input.path() / "input";
  auto out = open_output(file);
  for (auto x : values) write_value(out, x);
  close_output(out);
  fs::path owned;
  {
    Workspace scratch;
    owned = scratch.path();
    auto result = external_sort(file, scratch, budget, fan);
    std::vector<Value> actual;
    auto in = open_input(result.file);
    Value x{};
    while (read_value(in, x)) actual.push_back(x);
    auto expected = values;
    std::sort(expected.begin(), expected.end());
    check(actual == expected);
    check(result.peak_records <= budget && result.peak_open_files <= fan + 1);
    check(std::distance(fs::directory_iterator(owned), fs::directory_iterator{}) == 1);
    if (values.size() > budget * fan) check(result.merge_passes > 1);
    rejects([&] { external_sort(file, scratch, budget, fan); });
  }
  check(!fs::exists(owned) && fs::exists(file));
}
int main() {
  try {
    verify({}, 2, 2); verify({-7}, 2, 2);
    verify({4, 4, 4, 4}, 2, 2);
    verify({std::numeric_limits<Value>::min(), 0, std::numeric_limits<Value>::max(), -1}, 2, 2);
    std::mt19937 random(445);
    std::vector<Value> values;
    for (int i = 0; i < 1003; ++i) values.push_back(static_cast<int>(random() % 101) - 50);
    for (auto budget : {2U, 7U, 32U}) for (auto fan : {2U, budget}) verify(values, budget, fan);
    Workspace input;
    auto file = input.path() / "bad";
    auto out = open_output(file); out.put('x'); close_output(out);
    fs::path owned;
    {
      Workspace scratch; owned = scratch.path();
      rejects([&] { external_sort(file, scratch, 2, 2); });
    }
    check(!fs::exists(owned));
    Workspace scratch;
    rejects([&] { external_sort(file, scratch, 1, 2); });
    rejects([&] { external_sort(file, scratch, 2, 3); });
    rejects([&] { external_sort(input.path() / "missing", scratch, 2, 2); });
    // A stream failure must not masquerade as clean EOF or a successful write.
    std::istringstream broken; broken.setstate(std::ios::badbit);
    Value x{}; rejects([&] { read_value(broken, x); });
    std::ostringstream sink; sink.setstate(std::ios::badbit);
    rejects([&] { write_value(sink, 1); });
    std::cout << "external_sort: all checks passed\n";
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

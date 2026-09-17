#include "external_sort.h"
#include <iostream>
int main() {
  try {
    tutorial::Workspace input, scratch;
    auto file = input.path() / "input";
    auto out = tutorial::open_output(file);
    for (auto x : {9, -1, 5, 5, 2, 8, 0, -3, 7, 4}) tutorial::write_value(out, x);
    tutorial::close_output(out);
    auto result = tutorial::external_sort(file, scratch, 3, 2);
    auto in = tutorial::open_input(result.file);
    tutorial::Value x{};
    while (tutorial::read_value(in, x)) std::cout << x << ' ';
    std::cout << "\nruns=" << result.initial_runs << " passes=" << result.merge_passes
              << " peak_records=" << result.peak_records << " open_files=" << result.peak_open_files << '\n';
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

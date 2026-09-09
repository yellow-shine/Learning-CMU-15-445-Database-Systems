#include "join.h"
#include <algorithm>
#include <iostream>
int main() {
    const tutorial::Table left{2, std::nullopt, 1, 2};
    const tutorial::Table right{2, 3, 2, std::nullopt, 2};
    auto result = tutorial::join(left, right);
    // Normalize only for presentation; SQL without ORDER BY has no order promise.
    std::sort(result.matches.begin(), result.matches.end());
    for (auto [l, r] : result.matches) std::cout << l << "," << r << "\n";
    std::cout << "build=" << (result.build_left ? "left" : "right")
              << " build_rows=" << result.build_rows << " probe_rows=" << result.probe_rows << "\n";
}

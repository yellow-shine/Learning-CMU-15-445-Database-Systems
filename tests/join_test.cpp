#include "join.h"
#include <algorithm>
#include <climits>
#include <iostream>
#include <random>
#include <stdexcept>

using namespace tutorial;
void require(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}
std::vector<Match> oracle(const Table &left, const Table &right) {
    std::vector<Match> expected;
    for (std::size_t i = 0; i < left.size(); ++i)
        for (std::size_t j = 0; j < right.size(); ++j)
            if (left[i].has_value() && right[j].has_value() && left[i].value() == right[j].value())
                expected.emplace_back(i, j);
    return expected;
}
void check(const Table &left, const Table &right) {
    const auto before_left = left, before_right = right;
    auto result = join(left, right);
    std::sort(result.matches.begin(), result.matches.end());
    require(result.matches == oracle(left, right), "RID multiset differs from oracle");
    require(left == before_left && right == before_right, "input modified");
    std::vector<int> keys;
    for (const auto &m : result.matches) keys.push_back(*left[m.first]);
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
    require(result.matched_groups == keys.size(), "matched group count");
    require(result.merge_comparisons <= left.size() + right.size(), "merge not linear");
}
int main() {
    try {
        check({}, {}); check({}, {1, std::nullopt}); check({1, std::nullopt}, {});
        check({1, 2}, {3, 4}); check({std::nullopt}, {std::nullopt});
        check({std::nullopt, std::nullopt}, {std::nullopt, 1});
        check({INT_MIN, 0, INT_MAX, -1}, {INT_MAX, INT_MIN, 0, -1});
        const std::vector<Match> six{{0,0},{0,1},{0,2},{1,0},{1,1},{1,2}};
        auto duplicate = join({7,7}, {7,7,7});
        std::sort(duplicate.matches.begin(), duplicate.matches.end());
        require(duplicate.matches == six, "2 x 3 duplicates must yield six distinct RID pairs");
        Table l{2, std::nullopt, 1, 2}, r{2, 3, 2, std::nullopt, 2};
        std::mt19937 random(445);
        for (int i = 0; i < 100; ++i) {
            std::shuffle(l.begin(), l.end(), random);
            std::shuffle(r.begin(), r.end(), random);
            check(l, r); check(r, l);
        }
        for (int trial = 0; trial < 1000; ++trial) {
            Table a(random() % 13), b(random() % 13);
            for (auto *table : {&a, &b})
                for (auto &key : *table) {
                    const auto value = random() % 6;
                    if (value != 0) key = static_cast<int>(value) - 3;
                }
            check(a, b);
        }
        std::cout << "join tests passed (edge cases, duplicates, 200 shuffles, 1000 differential cases)\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}

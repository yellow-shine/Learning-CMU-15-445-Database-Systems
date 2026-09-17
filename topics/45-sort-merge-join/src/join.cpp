#include "join.h"
#include <algorithm>

namespace tutorial {
Result join(const Table &left, const Table &right) {
    Result out;
    using Entry = std::pair<int, std::size_t>;
    std::vector<Entry> ls, rs;
    for (std::size_t i = 0; i < left.size(); ++i)
        if (left[i]) ls.emplace_back(*left[i], i);
    for (std::size_t i = 0; i < right.size(); ++i)
        if (right[i]) rs.emplace_back(*right[i], i);
    // This is in-memory sorting, not external sorting.
    std::sort(ls.begin(), ls.end());
    std::sort(rs.begin(), rs.end());
    std::size_t l = 0, r = 0;
    while (l < ls.size() && r < rs.size()) {
        ++out.merge_comparisons;
        if (ls[l].first < rs[r].first) { ++l; continue; }
        if (ls[l].first > rs[r].first) { ++r; continue; }
        const int key = ls[l].first;
        auto lend = l + 1, rend = r + 1;
        while (lend < ls.size() && ls[lend].first == key) ++lend;
        while (rend < rs.size() && rs[rend].first == key) ++rend;
        ++out.matched_groups;
        // The whole duplicate group must be multiplied, not zipped.
        for (auto i = l; i < lend; ++i)
            for (auto j = r; j < rend; ++j)
                out.matches.emplace_back(ls[i].second, rs[j].second);
        l = lend;
        r = rend;
    }
    return out;
}
} // namespace tutorial

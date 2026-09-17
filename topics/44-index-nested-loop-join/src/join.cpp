#include "join.h"
#include <algorithm>

namespace tutorial {
Result join(const Table &left, const Table &right) {
    Result out;
    // A read-only secondary index: sorted (key, RID), with every duplicate.
    // ponytail: rebuild per call; retain an index snapshot for repeated queries.
    std::vector<std::pair<int, std::size_t>> index;
    for (std::size_t r = 0; r < right.size(); ++r)
        if (right[r]) index.emplace_back(*right[r], r);
    std::sort(index.begin(), index.end());
    out.index_entries = index.size();
    for (std::size_t l = 0; l < left.size(); ++l) {
        if (!left[l]) continue;
        ++out.probes;
        auto it = std::lower_bound(index.begin(), index.end(), *left[l],
            [](const auto &entry, int key) { return entry.first < key; });
        for (; it != index.end() && it->first == *left[l]; ++it) {
            ++out.candidate_visits;
            const auto r = it->second;
            // Fetch the candidate from the inner table by RID, not a full scan.
            if (right[r] && *right[r] == *left[l]) out.matches.emplace_back(l, r);
        }
    }
    return out;
}
} // namespace tutorial

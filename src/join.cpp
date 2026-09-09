#include "join.h"
#include <unordered_map>

namespace tutorial {
Result join(const Table &left, const Table &right) {
    Result out;
    // Choose by total input cardinality; ties build the left side.
    out.build_left = left.size() <= right.size();
    const auto &build = out.build_left ? left : right;
    const auto &probe = out.build_left ? right : left;
    std::unordered_map<int, std::vector<std::size_t>> buckets;
    for (std::size_t b = 0; b < build.size(); ++b) {
        if (!build[b]) continue;
        ++out.build_rows;
        buckets[*build[b]].push_back(b);
    }
    for (std::size_t p = 0; p < probe.size(); ++p) {
        if (!probe[p]) continue;
        ++out.probe_rows;
        const auto found = buckets.find(*probe[p]);
        if (found == buckets.end()) continue;
        for (auto b : found->second) {
            // Physical build/probe orientation must not change logical columns.
            if (out.build_left) out.matches.emplace_back(b, p);
            else out.matches.emplace_back(p, b);
        }
    }
    return out;
}
} // namespace tutorial

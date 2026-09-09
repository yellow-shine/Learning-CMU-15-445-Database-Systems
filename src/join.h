#pragma once
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace tutorial {
// RID is the position in the input snapshot, not a user-supplied key.
using Row = std::optional<int>;
using Table = std::vector<Row>;
using Match = std::pair<std::size_t, std::size_t>;
struct Result {
    std::vector<Match> matches; // Always (left RID, right RID); bag semantics.
    bool build_left = false;
    std::size_t build_rows = 0, probe_rows = 0;
};
Result join(const Table &left, const Table &right);
} // namespace tutorial

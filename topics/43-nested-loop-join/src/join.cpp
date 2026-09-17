#include "join.h"

namespace tutorial {
Result join(const Table &left, const Table &right) {
    Result out;
    for (std::size_t l = 0; l < left.size(); ++l) {
        for (std::size_t r = 0; r < right.size(); ++r) {
            ++out.comparisons;
            // SQL equality with NULL is UNKNOWN, never TRUE.
            if (left[l] && right[r] && *left[l] == *right[r])
                out.matches.emplace_back(l, r);
        }
    }
    return out;
}
} // namespace tutorial

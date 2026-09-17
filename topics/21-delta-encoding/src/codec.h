#pragma once
#include "bytes.h"
namespace tutorial {
using Values = std::vector<std::int64_t>;
// Differences outside int64_t are rejected, never wrapped or silently truncated.
Bytes encode(const Values &values);
Values decode(const Bytes &bytes);
} // namespace tutorial

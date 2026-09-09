#pragma once
#include "bytes.h"
namespace tutorial {
using Values = std::vector<std::uint64_t>;
Bytes encode(const Values &values);
Values decode(const Bytes &bytes);
} // namespace tutorial

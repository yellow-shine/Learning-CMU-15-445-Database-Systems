#pragma once
#include "bytes.h"
namespace tutorial {
using Values = std::vector<std::uint64_t>;
unsigned bit_width(std::uint64_t value);
unsigned required_width(const Values &values);
Bytes encode(const Values &values, unsigned width);
Bytes encode(const Values &values);
Values decode(const Bytes &bytes);
} // namespace tutorial

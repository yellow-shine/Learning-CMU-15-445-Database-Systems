#pragma once
#include "bytes.h"
#include <string>
namespace tutorial {
using Values = std::vector<std::string>;
constexpr std::size_t max_string_bytes = 16 * 1024 * 1024;
Bytes encode(const Values &values);
Values decode(const Bytes &bytes);
// Returns zero-based row positions, comparing integer IDs rather than row strings.
std::vector<std::size_t> filter_equal(const Bytes &bytes, const std::string &key);
} // namespace tutorial

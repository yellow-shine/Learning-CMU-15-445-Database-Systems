#pragma once
#include <array>
#include <cstddef>
namespace storage {
constexpr std::size_t page_size=256;
using Page=std::array<unsigned char,page_size>;
}

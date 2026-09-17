#pragma once
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace tutorial {
using Bytes = std::vector<std::uint8_t>;
// Bounded teaching blocks: reject hostile counts before allocation.
constexpr std::size_t max_items = 1000000;
inline void require(bool condition) {
  if (!condition) throw std::invalid_argument("invalid codec input");
}
inline void put(Bytes &out, std::uint64_t value, unsigned bytes) {
  for (unsigned i = 0; i < bytes; ++i) {
    out.push_back(static_cast<std::uint8_t>(value & 255U));
    value >>= 8U;
  }
}
struct Reader {
  const Bytes &data;
  std::size_t pos = 0;
  std::uint64_t get(unsigned bytes) {
    require(bytes <= 8 && bytes <= data.size() - pos);
    std::uint64_t value = 0;
    for (unsigned i = 0; i < bytes; ++i)
      value |= std::uint64_t(data[pos++]) << (8U * i);
    return value;
  }
  std::size_t count() {
    auto n = get(4);
    require(n <= max_items);
    return static_cast<std::size_t>(n);
  }
  void end() const { require(pos == data.size()); }
};
} // namespace tutorial

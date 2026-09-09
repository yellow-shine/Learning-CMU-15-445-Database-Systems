#include "codec.h"
#include <algorithm>
namespace tutorial {
unsigned bit_width(std::uint64_t value) {
  unsigned width = 0;
  while (value != 0) { ++width; value >>= 1U; }
  return width;
}
unsigned required_width(const Values &values) {
  unsigned width = 0;
  for (auto value : values) width = std::max(width, bit_width(value));
  return width;
}
Bytes encode(const Values &values) { return encode(values, required_width(values)); }
Bytes encode(const Values &values, unsigned width) {
  require(values.size() <= max_items && width <= 64);
  require(required_width(values) <= width);
  Bytes out;
  put(out, values.size(), 4);
  put(out, width, 1);
  out.resize(5 + (values.size() * width + 7) / 8, 0);
  std::size_t bit = 0;
  // ponytail: bit-at-a-time O(n*w); use word/SIMD packing only for measured throughput needs.
  for (auto value : values)
    for (unsigned b = 0; b < width; ++b, ++bit)
      out[5 + bit/8] |= static_cast<std::uint8_t>(((value >> b) & 1U) << (bit%8));
  return out;
}
Values decode(const Bytes &bytes) {
  Reader in{bytes};
  const auto n = in.count();
  const auto width = static_cast<unsigned>(in.get(1));
  require(width <= 64);
  const auto bits = n * width;
  require(bytes.size() == 5 + (bits + 7)/8);
  if (bits%8 != 0) require((bytes.back() >> (bits%8)) == 0);
  Values out(n, 0);
  std::size_t bit = 0;
  for (auto &value : out)
    for (unsigned b = 0; b < width; ++b, ++bit)
      value |= std::uint64_t((bytes[5 + bit/8] >> (bit%8)) & 1U) << b;
  return out;
}
} // namespace tutorial

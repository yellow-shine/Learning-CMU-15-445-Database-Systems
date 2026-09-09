#include "codec.h"
#include <limits>
namespace tutorial {
namespace {
constexpr auto low = std::numeric_limits<std::int64_t>::min();
constexpr auto high = std::numeric_limits<std::int64_t>::max();
std::uint64_t zigzag(std::int64_t value) {
  // -(MIN+1) is representable; no signed left shift or negation of MIN.
  return value < 0 ? std::uint64_t(-(value + 1)) * 2U + 1U
                   : std::uint64_t(value) * 2U;
}
std::int64_t unzigzag(std::uint64_t value) {
  const auto half = static_cast<std::int64_t>(value >> 1U);
  return value & 1U ? -1 - half : half;
}
void varint(Bytes &out, std::uint64_t value) {
  do {
    const auto payload = static_cast<std::uint8_t>(value & 127U);
    value >>= 7U;
    out.push_back(static_cast<std::uint8_t>(payload | (value ? 128U : 0U)));
  } while (value);
}
std::uint64_t varint(Reader &in) {
  std::uint64_t value = 0;
  for (unsigned i = 0; i < 10; ++i) {
    const auto byte = in.get(1);
    const auto payload = byte & 127U;
    if (i == 9) require(byte <= 1); // only bit 63 remains; no continuation
    value |= payload << (7U * i);
    if ((byte & 128U) == 0) {
      require(i == 0 || payload != 0); // reject overlong representation
      return value;
    }
  }
  throw std::invalid_argument("invalid varint");
}
} // namespace
Bytes encode(const Values &values) {
  require(values.size() <= max_items);
  Bytes out;
  put(out, values.size(), 4);
  if (values.empty()) return out;
  varint(out, zigzag(values.front()));
  for (std::size_t i = 1; i < values.size(); ++i) {
    const auto previous = values[i-1], current = values[i];
    require(!(previous > 0 && current < low + previous));
    require(!(previous < 0 && current > high + previous));
    varint(out, zigzag(current - previous));
  }
  return out;
}
Values decode(const Bytes &bytes) {
  Reader in{bytes};
  const auto n = in.count();
  Values out;
  if (n != 0) out.push_back(unzigzag(varint(in)));
  while (out.size() < n) {
    const auto delta = unzigzag(varint(in));
    const auto previous = out.back();
    require(!(delta > 0 && previous > high - delta));
    require(!(delta < 0 && previous < low - delta));
    out.push_back(previous + delta);
  }
  in.end();
  return out;
}
} // namespace tutorial

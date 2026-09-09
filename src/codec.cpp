#include "codec.h"
namespace tutorial {
Bytes encode(const Values &values) {
  require(values.size() <= max_items);
  Bytes out;
  put(out, values.size(), 4);
  for (std::size_t begin = 0; begin < values.size();) {
    std::size_t end = begin + 1;
    while (end < values.size() && values[end] == values[begin]) ++end;
    put(out, values[begin], 8);
    put(out, end - begin, 4);
    begin = end;
  }
  return out;
}
Values decode(const Bytes &bytes) {
  Reader in{bytes};
  const auto n = in.count();
  Values out;
  // Validate each run before expansion; no trusting encoded run lengths.
  while (out.size() < n) {
    const auto value = in.get(8);
    const auto count = in.get(4);
    require(count > 0 && count <= n - out.size());
    require(out.empty() || out.back() != value); // canonical maximal runs
    out.insert(out.end(), static_cast<std::size_t>(count), value);
  }
  in.end();
  return out;
}
} // namespace tutorial

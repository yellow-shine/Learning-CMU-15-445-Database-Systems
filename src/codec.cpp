#include "codec.h"
#include <unordered_map>
#include <unordered_set>
namespace tutorial {
namespace {
struct Block { Values dictionary; std::vector<std::uint32_t> ids; };
Block parse(const Bytes &bytes) {
  Reader in{bytes};
  const auto n = in.count();
  const auto d = in.count();
  require(d <= n && (n == 0 || d != 0));
  Block block;
  std::unordered_set<std::string> unique;
  std::size_t total = 0;
  for (std::size_t i = 0; i < d; ++i) {
    const auto length = in.get(4);
    require(length <= max_string_bytes - total && length <= bytes.size() - in.pos);
    std::string value(bytes.begin() + static_cast<std::ptrdiff_t>(in.pos),
                      bytes.begin() + static_cast<std::ptrdiff_t>(in.pos + length));
    in.pos += static_cast<std::size_t>(length);
    total += static_cast<std::size_t>(length);
    require(unique.insert(value).second);
    block.dictionary.push_back(std::move(value));
  }
  require(bytes.size() - in.pos == n * 4);
  std::size_t expanded = 0;
  for (std::size_t i = 0; i < n; ++i) {
    const auto id = in.get(4);
    require(id < d);
    const auto length = block.dictionary[static_cast<std::size_t>(id)].size();
    require(length <= max_string_bytes - expanded);
    expanded += length;
    block.ids.push_back(static_cast<std::uint32_t>(id));
  }
  in.end();
  return block;
}
} // namespace
Bytes encode(const Values &values) {
  require(values.size() <= max_items);
  Values dictionary;
  std::unordered_map<std::string, std::uint32_t> lookup;
  std::vector<std::uint32_t> ids;
  std::size_t total = 0;
  for (const auto &value : values) {
    require(value.size() <= max_string_bytes - total);
    total += value.size();
    const auto inserted = lookup.emplace(value, static_cast<std::uint32_t>(dictionary.size()));
    if (inserted.second) dictionary.push_back(value);
    ids.push_back(inserted.first->second);
  }
  Bytes out;
  put(out, values.size(), 4);
  put(out, dictionary.size(), 4);
  for (const auto &value : dictionary) {
    put(out, value.size(), 4);
    out.insert(out.end(), value.begin(), value.end());
  }
  for (auto id : ids) put(out, id, 4);
  return out;
}
Values decode(const Bytes &bytes) {
  const auto block = parse(bytes);
  Values out;
  for (auto id : block.ids) out.push_back(block.dictionary[id]);
  return out;
}
std::vector<std::size_t> filter_equal(const Bytes &bytes, const std::string &key) {
  const auto block = parse(bytes); // Validate even when key is absent.
  std::size_t target = block.dictionary.size();
  for (std::size_t i = 0; i < block.dictionary.size(); ++i)
    if (block.dictionary[i] == key) { target = i; break; }
  std::vector<std::size_t> rows;
  if (target == block.dictionary.size()) return rows;
  for (std::size_t i = 0; i < block.ids.size(); ++i)
    if (block.ids[i] == target) rows.push_back(i);
  return rows;
}
} // namespace tutorial

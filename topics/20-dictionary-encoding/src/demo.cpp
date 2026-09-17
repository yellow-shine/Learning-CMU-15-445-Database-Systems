#include "codec.h"
#include <iostream>
int main() {
  const tutorial::Values values{"database", "database", "storage", "database", "storage", "database"};
  const auto bytes = tutorial::encode(values);
  tutorial::Bytes raw;
  tutorial::put(raw, values.size(), 4);
  for (const auto &value : values) {
    tutorial::put(raw, value.size(), 4);
    raw.insert(raw.end(), value.begin(), value.end());
  }
  std::vector<std::size_t> reference;
  for (std::size_t i = 0; i < values.size(); ++i)
    if (values[i] == "database") reference.push_back(i);
  const auto rows = tutorial::filter_equal(bytes, "database");
  std::cout << "dictionary: 0=database 1=storage\nids: 0 0 1 0 1 0\nraw="
            << raw.size() << " encoded=" << bytes.size() << " bytes\nmatching rows:";
  for (auto row : rows) std::cout << ' ' << row;
  std::cout << '\n';
  return tutorial::decode(bytes) == values && rows == reference ? 0 : 1;
}

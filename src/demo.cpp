#include "codec.h"
#include <iostream>
int main() {
  const tutorial::Values values{1000,1001,1003,1002,1002};
  const auto bytes = tutorial::encode(values);
  std::cout << "first=1000 deltas: 1 2 -1 0\nraw=" << values.size()*8
            << " encoded=" << bytes.size() << " bytes\npayload:";
  for (std::size_t i = 4; i < bytes.size(); ++i) std::cout << ' ' << unsigned(bytes[i]);
  std::cout << '\n';
  return tutorial::decode(bytes) == values ? 0 : 1;
}

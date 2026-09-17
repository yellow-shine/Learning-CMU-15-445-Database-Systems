#include "codec.h"
#include <iostream>
int main() {
  const tutorial::Values values{1,7,3,5};
  const auto bytes = tutorial::encode(values);
  std::cout << "width=" << tutorial::required_width(values) << "\nraw="
            << values.size()*8 << " encoded=" << bytes.size() << " bytes\npayload:";
  for (std::size_t i=5; i<bytes.size(); ++i) std::cout << ' ' << unsigned(bytes[i]);
  std::cout << '\n';
  return tutorial::decode(bytes) == values ? 0 : 1;
}

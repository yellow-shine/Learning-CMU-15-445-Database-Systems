#include "codec.h"
#include <iostream>
int main() {
  const tutorial::Values values{7,7,7,7,7,7,7,7,9,9,9,9};
  const auto bytes = tutorial::encode(values);
  std::cout << "runs: (7,8) (9,4)\nraw=" << values.size()*8
            << " encoded=" << bytes.size() << " bytes\n";
  const auto restored = tutorial::decode(bytes);
  for (auto value : restored) std::cout << value << ' ';
  std::cout << '\n';
  return restored == values ? 0 : 1;
}

#include "page_buffer.h"
#include <iostream>
PageBuffer read_page() {
  PageBuffer buffer(4);
  buffer.at(0) = 42;
  return buffer;  // NRVO when available; otherwise the move constructor.
}
int main() {
  auto source = read_page();
  PageBuffer destination(std::move(source));
  std::cout << "construct move: source=" << source.size() << ", destination=" << destination.size() << '\n';
  PageBuffer slot(8);
  slot = std::move(destination);
  std::cout << "assign move: source=" << destination.size() << ", first=" << static_cast<int>(slot.at(0)) << '\n';
  try { source.at(0); }
  catch (const std::out_of_range &) { std::cout << "empty access rejected\n"; }
}

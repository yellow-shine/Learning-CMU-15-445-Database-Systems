#pragma once
#include <stdexcept>
#include <string>
inline void check(bool condition, const std::string &message) {
  if (!condition) throw std::runtime_error(message);
}
template <class F> void rejects(F action) {
  bool rejected = false;
  try { action(); } catch (const std::invalid_argument &) { rejected = true; }
  check(rejected, "expected invalid_argument");
}

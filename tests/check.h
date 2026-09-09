#pragma once
#include <stdexcept>
#define CHECK(...) do { if (!(__VA_ARGS__)) throw std::runtime_error("check failed: " #__VA_ARGS__); } while (false)
template<class F> void rejects(F f) {
  bool rejected = false;
  try { f(); } catch (const std::invalid_argument &) { rejected = true; }
  CHECK(rejected);
}

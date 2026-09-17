#pragma once
#include <stdexcept>
#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("check failed: " #expr); } while (false)
template<class F> void rejects(F f) { bool failed = false; try { f(); } catch (const std::exception&) { failed = true; } CHECK(failed); }

#pragma once
#include <stdexcept>
#include <iostream>
#define CHECK(x) do { if (!(x)) throw std::runtime_error("check failed: " #x); } while(false)
template<class F> void rejects(F f) { bool caught=false; try { f(); } catch(const std::exception&) { caught=true; } CHECK(caught); }

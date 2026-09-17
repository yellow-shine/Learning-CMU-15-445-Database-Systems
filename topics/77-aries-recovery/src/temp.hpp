#pragma once
#include "wal.hpp"
namespace tiny {
struct Temp {
  std::string path;
  Temp() { char pattern[]="/tmp/cmu445-wal-XXXXXX"; auto p=::mkdtemp(pattern); require(p,"mkdtemp failed"); path=p; }
  ~Temp() { std::error_code ec; std::filesystem::remove_all(path,ec); }
};
}

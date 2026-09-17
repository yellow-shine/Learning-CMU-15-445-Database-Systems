#include "file_guard.h"
#include "temp_directory.h"
#include <fstream>
#include <iostream>
int main() {
  TempDirectory temp;
  auto path = (temp.path / "page").string();
  { FileGuard file(path); file.write("page 7\n"); file.close(); }
  std::ifstream in(path);
  std::string line;
  std::getline(in, line);
  std::cout << "normal: " << line << '\n';
  try { FileGuard file((temp.path / "partial").string()); file.write("partial");
    throw std::runtime_error("executor failed");
  } catch (const std::runtime_error &) { std::cout << "exception: unwound\n"; }
  try { FileGuard file(""); }
  catch (const std::runtime_error &) { std::cout << "open failure: no owned resource\n"; }
}

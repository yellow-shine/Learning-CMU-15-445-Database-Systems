#include "page_file.h"
#include "temp_dir.h"
#include <iostream>
int main() {
  TempDir dir;
  { storage::PageFile f(dir.path/"pages", true);
    storage::Page p{}; p[0]=42; f.append(p); f.append(); }
  storage::PageFile f(dir.path/"pages");
  std::cout << "pages=" << f.size() << " page0[0]=" << int(f.read(0)[0]) << " page1[0]=" << int(f.read(1)[0]) << '\n';
}

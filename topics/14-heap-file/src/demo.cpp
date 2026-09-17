#include "heap_file.h"
#include "temp_dir.h"
#include <iostream>
int main() {
  TempDir dir; storage::RID second{};
  { storage::HeapFile h(dir.path/"heap",true); h.insert(std::string(140,'A')); second=h.insert(std::string(140,'B'));
    std::cout<<"pages="<<h.pages()<<" rid="<<second.page<<":"<<second.slot<<'\n'; }
  storage::HeapFile h(dir.path/"heap");
  std::cout<<"reopen rows="<<h.scan().size()<<" second bytes="<<h.get(second)->size()<<'\n';
}

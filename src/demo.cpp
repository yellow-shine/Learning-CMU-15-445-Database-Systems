#include "column_store.h"
#include <iostream>
int main() {
  storage::ColumnStore store({{1,10,20},{2,90,30},{3,80,40}}); std::size_t reads=0;
  auto positions=store.select(80,reads);
  std::cout<<"rows="<<store.size()<<" selected="<<positions.size()<<" scan reads="<<reads<<'\n';
  auto ids=store.project_ids(positions,reads);
  std::cout<<"ids="<<ids[0]<<","<<ids[1]<<" total reads="<<reads<<'\n';
}

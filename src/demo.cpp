#include "row_store.h"
#include <iostream>
int main() {
  storage::RowStore store({{1,10,20},{2,90,30},{3,80,40}}); std::size_t reads=0;
  auto positions=store.select(80,reads);
  std::cout<<"rows="<<store.size()<<" bytes="<<store.bytes().size()<<" selected="<<positions.size()<<" scan reads="<<reads<<'\n';
  reads=0; auto row=store.get(positions[0],reads);
  std::cout<<"first="<<row.id<<","<<row.score<<","<<row.age<<" row reads="<<reads<<'\n';
}

#include "pax_store.h"
#include <iostream>
int main() {
  std::vector<storage::Row> rows; for(std::uint32_t i=0;i<12;++i) rows.push_back({i,i*10,20+i});
  storage::PaxStore store(rows); std::size_t reads=0; auto positions=store.select(90,reads);
  std::cout<<"pages="<<store.pages().size()<<" counts="<<int(store.pages()[0][4])<<","<<int(store.pages()[1][4])<<" scan reads="<<reads<<'\n';
  reads=0; auto row=store.get(positions[1],reads);
  std::cout<<"selected="<<positions.size()<<" row="<<row.id<<","<<row.score<<","<<row.age<<" row reads="<<reads<<'\n';
  std::cout<<"mini-page starts="<<store.offset(0,0)<<","<<store.offset(1,0)<<","<<store.offset(2,0)<<'\n';
}

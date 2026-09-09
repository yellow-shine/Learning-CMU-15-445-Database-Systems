#include "row_store.h"
#include "check.h"
int main() { try {
  using namespace storage; std::size_t reads=0; RowStore empty({}); CHECK(empty.select(0,reads).empty()); CHECK(reads==0);
  rejects([&]{empty.get(0,reads);}); CHECK(reads==0);
  std::vector<Row> rows;
  for(std::uint32_t i=0;i<101;++i) rows.push_back({i,i%11,i*3});
  rows.push_back({UINT32_MAX,UINT32_MAX,0}); RowStore store(rows);
  CHECK(store.bytes().size()==12*rows.size()); CHECK(store.bytes()[12]==1); CHECK(store.bytes()[16]==1);
  for(std::size_t i=0;i<rows.size();++i) CHECK(store.get(i,reads)==rows[i]);
  for(auto threshold:{0u,5u,11u,UINT32_MAX}) {
    reads=0; auto got=store.select(threshold,reads); std::vector<std::size_t> expected;
    for(std::size_t i=0;i<rows.size();++i) if(rows[i].score>=threshold) expected.push_back(i);
    CHECK(got==expected); CHECK(reads==3*rows.size());
  }
  reads=0; rejects([&]{store.get(store.size(),reads);}); rejects([&]{store.get(SIZE_MAX,reads);}); CHECK(reads==0);
  std::cout<<"row store checks passed\n";
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;} }

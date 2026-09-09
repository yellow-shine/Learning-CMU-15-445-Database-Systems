#include "column_store.h"
#include "check.h"
int main() { try {
  using namespace storage; std::size_t reads=0; ColumnStore empty({}); CHECK(empty.select(0,reads).empty());
  CHECK(empty.project_ids({},reads).empty()); CHECK(reads==0); rejects([&]{empty.get(0,reads);});
  std::vector<Row> rows; for(std::uint32_t i=0;i<101;++i) rows.push_back({i%7,i%11,i*3});
  rows.push_back({UINT32_MAX,UINT32_MAX,0}); ColumnStore store(rows);
  for(std::size_t i=0;i<rows.size();++i) { CHECK(store.get(i,reads)==rows[i]);
    CHECK(store.column(0)[i]==rows[i].id); CHECK(store.column(1)[i]==rows[i].score); CHECK(store.column(2)[i]==rows[i].age); }
  for(auto threshold:{0u,5u,11u,UINT32_MAX}) {
    reads=0; auto got=store.select(threshold,reads); std::vector<std::size_t> expected; std::vector<std::uint32_t> ids;
    for(std::size_t i=0;i<rows.size();++i) if(rows[i].score>=threshold) {expected.push_back(i);ids.push_back(rows[i].id);}
    CHECK(got==expected); CHECK(reads==rows.size()); CHECK(store.project_ids(got,reads)==ids); CHECK(reads==rows.size()+ids.size());
  }
  reads=0; CHECK(store.project_ids({2,0,2},reads)==std::vector<std::uint32_t>({2,0,2})); CHECK(reads==3);
  reads=0; rejects([&]{store.project_ids({0,SIZE_MAX},reads);}); rejects([&]{store.get(store.size(),reads);});
  rejects([&]{store.column(3);}); CHECK(reads==0);
  std::cout<<"column store checks passed\n";
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;} }

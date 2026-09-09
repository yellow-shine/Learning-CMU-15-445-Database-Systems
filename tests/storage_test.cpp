#include "pax_store.h"
#include "check.h"
int main() { try {
  using namespace storage;
  CHECK(PaxStore::offset(0,0)==8); CHECK(PaxStore::offset(0,9)+4==48);
  CHECK(PaxStore::offset(1,0)==48); CHECK(PaxStore::offset(1,9)+4==88);
  CHECK(PaxStore::offset(2,0)==88); CHECK(PaxStore::offset(2,9)+4==128);
  rejects([]{PaxStore::offset(3,0);}); rejects([]{PaxStore::offset(0,10);});
  for(std::size_t n:{0u,1u,9u,10u,11u,20u,21u,101u}) {
    std::vector<Row> rows; for(std::size_t i=0;i<n;++i) rows.push_back({static_cast<std::uint32_t>(i%7),static_cast<std::uint32_t>(i%11),UINT32_MAX});
    PaxStore store(rows); CHECK(store.pages().size()==(n+9)/10); auto loaded=PaxStore::from_pages(store.pages());
    std::size_t reads=0; CHECK(loaded.size()==n);
    for(std::size_t i=0;i<n;++i) CHECK(loaded.get(i,reads)==rows[i]);
    CHECK(reads==3*n);
    for(auto threshold:{0u,5u,11u,UINT32_MAX}) {
      reads=0; auto got=loaded.select(threshold,reads); std::vector<std::size_t> expected;
      for(std::size_t i=0;i<n;++i) if(rows[i].score>=threshold) expected.push_back(i);
      CHECK(got==expected); CHECK(reads==n);
    }
    reads=0; rejects([&]{loaded.get(n,reads);}); rejects([&]{loaded.get(SIZE_MAX,reads);}); CHECK(reads==0);
  }
  PaxStore one({{0x12345678,90,UINT32_MAX}}); auto pages=one.pages(); CHECK(pages[0][8]==0x78); CHECK(pages[0][11]==0x12);
  CHECK(pages[0][48]==90); CHECK(pages[0][88]==255);
  for(auto pos:{0,1,2,3,4,5,6,7}) { auto bad=pages;bad[0][pos]=255;rejects([&]{PaxStore::from_pages(bad);}); }
  auto bad=pages;bad[0][4]=0;rejects([&]{PaxStore::from_pages(bad);});
  for(auto pos:{12,52,92,127}) {bad=pages;bad[0][pos]=1;rejects([&]{PaxStore::from_pages(bad);});}
  bad=pages; bad.push_back(pages[0]); rejects([&]{PaxStore::from_pages(bad);});
  std::cout<<"PAX checks passed\n";
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;} }

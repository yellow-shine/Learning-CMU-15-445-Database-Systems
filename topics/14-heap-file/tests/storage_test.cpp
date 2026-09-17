#include "heap_file.h"
#include "temp_dir.h"
#include "check.h"
int main() { try {
  using namespace storage; TempDir d; auto path=d.path/"heap";
  std::vector<RID> ids; std::vector<std::string> values;
  { HeapFile h(path,true); CHECK(h.scan().empty());
    rejects([&]{h.get({0,0});}); rejects([&]{h.insert("");});
    for(int i=0;i<40;++i) { values.push_back(std::string(90+ i%30,char('a'+i%26))); ids.push_back(h.insert(values.back())); }
    CHECK(h.pages()>1);
    for(std::size_t i=0;i<ids.size();++i) CHECK(h.get(ids[i])==values[i]);
    auto before=h.scan(); auto pages=h.pages();
    rejects([&]{h.insert(std::string(245,'x'));}); CHECK(h.pages()==pages); CHECK(h.scan().size()==before.size());
    CHECK(!h.update(ids[0],std::string(256,'x'))); CHECK(h.get(ids[0])==values[0]);
    CHECK(h.update(ids[1],"changed")); values[1]="changed"; h.erase(ids[0]); CHECK(!h.get(ids[0]));
    rejects([&]{h.get({UINT64_MAX,0});}); rejects([&]{h.get({0,999});});
  }
  { HeapFile h(path); auto rows=h.scan(); CHECK(rows.size()==39); CHECK(!h.get(ids[0]));
    for(std::size_t i=1;i<ids.size();++i) CHECK(h.get(ids[i])==values[i]);
    for(const auto& row:rows) CHECK(h.get(row.first)==row.second);
    auto id=h.insert("new"); CHECK(!(id.page==ids[0].page && id.slot==ids[0].slot));
  }
  { PageFile f(path); auto p=f.read(0); p[0]=0; f.write(0,p); }
  rejects([&]{HeapFile corrupt(path);});
  std::filesystem::resize_file(path,1); rejects([&]{HeapFile short_file(path);});
  std::cout<<"heap checks passed\n";
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;} }

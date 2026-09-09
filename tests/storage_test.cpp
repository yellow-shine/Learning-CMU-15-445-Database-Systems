#include "page_file.h"
#include "temp_dir.h"
#include "check.h"
int main() { try {
  TempDir d; auto path=d.path/"pages";
  rejects([&]{storage::PageFile missing(path);});
  { storage::PageFile f(path,true); CHECK(f.size()==0);
    rejects([&]{f.read(0);}); rejects([&]{f.write(0,{});});
    storage::Page a{}; for(std::size_t i=0;i<a.size();++i) a[i]=static_cast<unsigned char>(i);
    CHECK(f.append(a)==0); CHECK(f.append()==1); CHECK(f.read(0)==a);
    a[255]=17; f.write(1,a); CHECK(f.read(1)==a);
    rejects([&]{f.read(UINT64_MAX);}); rejects([&]{f.write(2,a);});
    rejects([&]{storage::PageFile duplicate(path,true);}); CHECK(f.read(1)==a);
  }
  { storage::PageFile f(path); CHECK(f.size()==2); CHECK(f.read(1)[255]==17);
    std::filesystem::resize_file(path,10); rejects([&]{f.read(1);}); }
  rejects([&]{storage::PageFile corrupt(path);});
  std::cout << "page file checks passed\n";
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; } }

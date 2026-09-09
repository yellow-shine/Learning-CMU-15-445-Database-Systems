#include "temp.hpp"
#include <iostream>
int main() {
  try {
    tiny::Temp dir;
    { tiny::Store db(dir.path); db.begin(1); db.update(1,0,40); db.commit(1);
      db.begin(2); db.update(2,1,20); db.update(2,1,99); db.flushPages(); }
    tiny::Store db(dir.path);
    db.recover([&](const char* phase) {
      if(std::string(phase)=="analysis")
        std::cout<<"analysis losers="<<db.transactionTable.size()<<" dirty="<<db.dirtyPageTable.size()<<'\n';
      if(std::string(phase)=="clr") {auto r=db.log.records.back();
        std::cout<<"CLR lsn="<<r.lsn<<" target="<<r.before<<" undoNextLSN="<<r.next<<'\n';}
    });
    std::cout<<"pages="<<db.pages[0].value<<','<<db.pages[1].value<<" redo="<<db.redone<<" undo="<<db.undone<<'\n';
    db.recover();
    std::cout<<"repeat redo="<<db.redone<<" undo="<<db.undone<<" skipped="<<db.skipped<<'\n';
  } catch(const std::exception& e) {std::cerr<<e.what()<<'\n'; return 1;}
}

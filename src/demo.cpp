#include "temp.hpp"
#include <iostream>
int main() {
 try {
  tiny::Temp dir;
  { tiny::Store db(dir.path); db.begin(1); db.update(1,0,10); db.commit(1); db.flushPages();
    db.begin(2); db.update(2,1,20); db.checkpoint(); auto cp=db.loadCheckpoint();
    std::cout<<"checkpoint cutoff="<<cp.cutoff<<" TT="<<cp.transactions.size()<<" DPT="<<cp.dirtyPages.size()<<" start="<<cp.start<<'\n';
    db.update(2,1,30); db.commit(2); }
  tiny::Store db(dir.path); db.recover();
  std::cout<<"recovery start="<<db.recoveryStart<<" scanned="<<db.replayScanned<<" pages="<<db.pages[0].value<<','<<db.pages[1].value<<'\n';
 } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}

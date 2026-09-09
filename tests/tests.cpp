#include "temp.hpp"
#include <functional>
#include <iostream>
#include <sys/wait.h>
using namespace tiny;
void child(const std::function<void()>& body) {
  auto pid=::fork(); require(pid>=0,"fork failed");
  if (!pid) { try { body(); ::_exit(0); } catch (...) { ::_exit(100); } }
  int status=0; require(::waitpid(pid,&status,0)==pid && WIFEXITED(status) && WEXITSTATUS(status)==0,"child failed");
}
void throws(const std::function<void()>& body) { bool caught=false; try { body(); } catch (const std::exception&) { caught=true; } require(caught,"expected rejection"); }
int main() {
 try {
  for (bool committed : {false,true}) for (bool stolen : {false,true}) {
    Temp dir;
    child([&] { Store db(dir.path); db.begin(1); require(db.update(1,0,42)==1,"first LSN");
      if(stolen) { db.flushPages(); require(db.log.durable>=db.pages[0].lsn,"WAL order"); }
      if(committed) { db.commit(1); require(db.log.durable==2,"commit durable"); }
      // _exit skips C++ destructors: no implicit clean shutdown.
    });
    child([&] { Store db(dir.path); db.recover(); require(db.pages[0].value==(committed?42:0),"crash outcome"); });
    child([&] { Store db(dir.path); db.recover(); require(db.pages[0].value==(committed?42:0),"repeat recovery"); });
  }
  { Temp dir; Store db(dir.path); db.recover(); require(db.pages[0].value==0,"empty");
    throws([&]{db.update(1,0,1);}); db.begin(1); throws([&]{db.update(1,4,1);});
    db.update(1,0,5); db.begin(2); throws([&]{db.update(2,0,9);}); db.commit(1); throws([&]{db.begin(1);}); }
  { Temp dir;
    { Store db(dir.path); db.begin(1); db.update(1,0,7); db.commit(1); }
    { Fd fd(dir.path+"/wal",O_WRONLY|O_APPEND); unsigned char tail[]={1,2,3}; writeAll(fd.value,tail,3); }
    { Store db(dir.path); require(db.log.trimmedTail,"tail detected"); db.recover(); require(db.pages[0].value==7,"valid prefix"); db.begin(2); db.update(2,1,8); db.commit(2); }
    { Store db(dir.path); db.recover(); require(db.pages[1].value==8,"append after tail repair"); }
    { Fd fd(dir.path+"/wal",O_WRONLY); unsigned char bad=9; writeAll(fd.value,&bad,1); }
    throws([&]{ Store db(dir.path); });
  }
  {Temp dir; {Store db(dir.path);db.begin(1);db.update(1,0,1);db.commit(1);}
    Store db(dir.path);throws([&]{db.begin(2);});db.recover();db.begin(2);throws([&]{db.recover();});}
  {Temp dir;
    {Store db(dir.path);db.begin(1);db.update(1,0,5);db.update(1,0,9);db.log.flush();}
    auto invalid=pack({Commit,3,1,0,0,0,1,0}); // Not the transaction's latest LSN.
    {Fd fd(dir.path+"/wal",O_WRONLY|O_APPEND);writeAll(fd.value,invalid.data(),invalid.size());}
    throws([&]{Store db(dir.path);});
  }
  std::cout<<"WAL crash, repetition, bounds, ownership, tail and checksum checks passed\n";
 } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}

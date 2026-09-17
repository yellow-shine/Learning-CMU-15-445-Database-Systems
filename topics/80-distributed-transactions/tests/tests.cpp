#include "two_phase.hpp"
#include "temp_directory.hpp"
#include "check.hpp"
#include <fstream>
#include <sys/wait.h>
using namespace tutorial;
void interrupt(const std::filesystem::path& dir,int stage,bool yes=true) {
 auto pid=::fork(); if(pid<0) io_error("fork");
 if(pid==0) {
  try {
   Protocol p(dir); p.prepare(0,true); p.prepare(1,yes);
   if(stage>=1) p.decide();
   if(stage>=2) p.deliver(0);
   ::_exit(0); // Abrupt process end: no C++ destructors or stream flushes.
  } catch(...) {::_exit(2);}
 }
 int status=0; while(::waitpid(pid,&status,0)<0) {if(errno!=EINTR) io_error("waitpid");}
 CHECK(WIFEXITED(status) && WEXITSTATUS(status)==0);
}
int main() {
 for(int stage=0;stage<3;++stage) {
  TempDirectory dir; interrupt(dir.path,stage);
  {
   Protocol p(dir.path);
   if(stage==0) {CHECK(p.decision()=='I'); CHECK(p.blocked(0) && p.blocked(1)); CHECK(!p.deliver(0)); p.recover_abort();}
   else {CHECK(p.decision()=='C'); p.recover_abort(); CHECK(p.decision()=='C');}
   p.deliver(0); p.deliver(1);
   CHECK(p.value(0)==(stage==0?10:7)); CHECK(p.value(1)==(stage==0?20:23));
  }
  for(int repeat=0;repeat<3;++repeat) {Protocol p(dir.path); p.deliver(0); p.deliver(1); CHECK(p.value(0)+p.value(1)==30); CHECK(!p.blocked(0));}
 }
 {TempDirectory dir; interrupt(dir.path,1,false); Protocol p(dir.path); CHECK(p.decision()=='A'); p.deliver(0); p.deliver(1); CHECK(p.value(0)==10 && p.value(1)==20);}
 {TempDirectory dir; Protocol p(dir.path); CHECK(p.prepare(0,true)); CHECK(p.prepare(0,false)); CHECK(p.decide()=='A'); CHECK(p.decide()=='A'); p.deliver(0); p.deliver(1); rejects([&]{p.prepare(0,true);}); rejects([&]{p.value(2);});}
 {TempDirectory dir; Journal p(dir.path/"p",false); rejects([&]{p.append('C');}); p.append('P'); p.append('C'); rejects([&]{p.append('A');});}
 for(const auto& bad: {"X","CC","CP","PAC","PP","\n"}) {
  TempDirectory dir; {std::ofstream out(dir.path/"participant0.log"); out<<bad;}
  rejects([&]{Protocol p(dir.path);});
 }
 {TempDirectory dir; {std::ofstream out(dir.path/"coordinator.log"); out<<'P';} rejects([&]{Protocol p(dir.path);});}
 {TempDirectory dir; rejects([&]{Protocol p(dir.path/"missing");});}
}

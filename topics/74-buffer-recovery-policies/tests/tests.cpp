#include "policies.hpp"
#include <iostream>
using namespace tiny;
int main() {
 try {
  for(bool steal:{false,true}) for(bool force:{false,true}) {
    BufferModel m(steal,force); m.begin(1); m.update(1,0,5); m.update(1,0,10); m.commit(1);
    m.begin(2); m.update(2,1,50); m.update(2,1,99);
    require(m.evict(1)==steal,"no-steal must refuse loser eviction"); m.crash();
    require(m.disk[0]==(force?10:0) && m.disk[1]==(steal?99:0),"policy effects");
    auto noUndo=m; noUndo.recover(false,!force); require((noUndo.disk[1]!=0)==steal,"undo necessity witness");
    auto noRedo=m; noRedo.recover(steal,false); require((noRedo.disk[0]!=10)==!force,"redo necessity witness");
    m.recover(steal,!force); require(m.disk==std::array<int,2>{10,0},"recovery");
    m.recover(steal,!force); require(m.disk==std::array<int,2>{10,0},"idempotence");
    BufferModel empty(steal,force); empty.crash(); empty.recover(steal,!force); require(empty.disk==std::array<int,2>{0,0},"empty");
    BufferModel clean(steal,force); clean.begin(1); clean.update(1,0,10); clean.commit(1); require(clean.evict(0),"committed eviction allowed");
  }
  BufferModel m(false,false); bool rejected=false;
  try {m.update(0,0,1);} catch(const std::exception&) {rejected=true;} require(rejected,"invalid tx");
  m.begin(1); m.update(1,0,1); m.begin(2); rejected=false;
  try {m.update(2,0,2);} catch(const std::exception&) {rejected=true;} require(rejected,"strict ownership");
  std::cout<<"four policies: effects, missing undo/redo counterexamples and repeated recovery passed\n";
 } catch(const std::exception& e) {std::cerr<<e.what()<<'\n'; return 1;}
}

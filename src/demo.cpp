#include "policies.hpp"
#include <iostream>
int main() {
  for (bool steal:{false,true}) for(bool force:{false,true}) {
    tiny::BufferModel m(steal,force); m.begin(1); m.update(1,0,10); m.commit(1);
    m.begin(2); m.update(2,1,99); m.evict(1); m.crash();
    std::cout<<"steal="<<steal<<" force="<<force<<" crash=["<<m.disk[0]<<','<<m.disk[1]<<"] undo="<<steal<<" redo="<<!force;
    m.recover(steal,!force); std::cout<<" recovered=["<<m.disk[0]<<','<<m.disk[1]<<"]\n";
  }
}

#include "two_phase.hpp"
#include "temp_directory.hpp"
#include <iostream>
int main() {
 tutorial::TempDirectory dir;
 {
  tutorial::Protocol p(dir.path); p.prepare(0,true); p.prepare(1,true);
  std::cout<<"prepared: blocked="<<p.blocked(0)<<'\n';
  p.decide(); p.deliver(0);
  std::cout<<"decision durable, first="<<p.value(0)<<" second="<<p.value(1)<<'\n';
 }
 tutorial::Protocol p(dir.path); p.deliver(0); p.deliver(1);
 std::cout<<"replayed: "<<p.value(0)<<' '<<p.value(1)<<'\n';
}

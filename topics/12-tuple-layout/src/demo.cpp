#include "tuple.h"
#include <iostream>
int main() {
  auto b=storage::encode({42,std::string("Ada")}); auto t=storage::decode(b);
  std::cout<<"bytes="<<b.size()<<" bitmap="<<int(b[1])<<" id="<<*t.id<<" name="<<*t.name<<'\n';
  auto null=storage::encode({std::nullopt,std::nullopt});
  std::cout<<"null bytes="<<null.size()<<" bitmap="<<int(null[1])<<'\n';
}

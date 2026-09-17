#include "slotted_page.h"
#include <iostream>
int main() {
  storage::SlottedPage p; auto a=*p.insert("cat"); auto b=*p.insert("elephant");
  p.erase(a); auto before=p.contiguous_free(); p.compact();
  std::cout<<"slot="<<b<<" value="<<*p.get(b)<<" free="<<before<<"->"<<p.contiguous_free()<<'\n';
  p.update(b,"ox"); std::cout<<"slot="<<b<<" value="<<*p.get(b)<<'\n';
}

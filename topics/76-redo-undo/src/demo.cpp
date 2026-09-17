#include "temp.hpp"
#include <iostream>
int main() {
  try {
    tiny::Temp dir;
    { tiny::Store db(dir.path); db.begin(1); db.update(1,0,40); db.commit(1);
      db.begin(2); db.update(2,1,99); db.flushPages();
      std::cout << "durableLSN=" << db.log.durable << " stolen page1=" << db.pages[1].value << " redo=" << db.redone << " undo=" << db.undone << '\n'; }
    tiny::Store db(dir.path); db.recover();
    std::cout << "recovered page0=" << db.pages[0].value << " page1=" << db.pages[1].value << " redo=" << db.redone << " undo=" << db.undone << '\n';
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

#include "buffer_pool.h"
#include "temp_file.h"
#include <iostream>
int main() {
  TempFile file(3); Disk disk(file.path()); BufferPool pool(disk, 1);
  pool.fetch(0).data[0] = 'A'; pool.unpin(0, true);
  pool.fetch(1); pool.unpin(1);
  std::cout << "reload page 0: " << pool.fetch(0).data[0] << '\n';
  pool.unpin(0); pool.flush_all();
}

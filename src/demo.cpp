#include "page_guard.h"
#include "temp_file.h"
#include <iostream>
int main() {
  TempFile file(2); Disk disk(file.path()); BufferPool pool(disk, 1);
  { WritePageGuard write(pool, 0); write.mutable_data()[0] = 'G'; }
  { ReadPageGuard other(pool, 1); }
  { ReadPageGuard read(pool, 0); std::cout << "guard reload: " << read.data()[0] << '\n'; }
  std::cout << "pins after scope: " << pool.pins(0) << '\n';
  pool.flush_all();
}

#include "disk_scheduler.h"
#include "temp_file.h"
#include <iostream>
int main() {
  TempFile file(2); Disk disk(file.path()); DiskScheduler scheduler(disk);
  Page page{}; page[0] = 'S';
  auto written = scheduler.write(0, page);
  auto read = scheduler.read(0); // FIFO: no need to wait for write before submission.
  written.get(); std::cout << "async read: " << read.get()[0] << '\n';
  auto bad = scheduler.read(99);
  try { bad.get(); } catch (const std::exception&) { std::cout << "I/O error delivered\n"; }
  scheduler.close();
}

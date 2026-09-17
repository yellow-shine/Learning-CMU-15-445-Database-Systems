#include "disk_scheduler.h"
#include "temp_file.h"
#include "check.h"
#include <vector>
int main() {
  TempFile file(8); Disk disk(file.path());
  DiskScheduler scheduler(disk);
  Page page{}; page[0] = 'Q';
  auto write = scheduler.write(0, page); page[0] = 'X';
  auto read = scheduler.read(0);
  CHECK(write.get()[0] == 'Q'); CHECK(read.get()[0] == 'Q'); // Owned copy, FIFO.
  auto bad = scheduler.read(8); rejects([&] { bad.get(); });
  CHECK(scheduler.read(0).get()[0] == 'Q'); // Worker survives an exception.
  std::vector<std::future<void>> producers;
  for (std::size_t i = 0; i < 8; ++i) {
    producers.push_back(std::async(std::launch::async, [&, i] {
      Page p{}; p[0] = static_cast<char>('a' + i);
      scheduler.write(i, p).get(); CHECK(scheduler.read(i).get() == p);
    }));
  }
  for (auto& future : producers) future.get();
  auto c1 = std::async(std::launch::async, [&] { scheduler.close(); });
  auto c2 = std::async(std::launch::async, [&] { scheduler.close(); }); c1.get(); c2.get();
  rejects([&] { scheduler.read(0); }); scheduler.close();
  std::vector<std::future<Page>> pending;
  {
    DiskScheduler draining(disk);
    for (int i = 0; i < 100; ++i) pending.push_back(draining.write(0, page));
  } // Destructor drains before Disk or request memory can disappear.
  for (auto& future : pending) CHECK(future.get() == page);
  {
    DiskScheduler errors(disk);
    std::filesystem::resize_file(file.path(), 1);
    auto short_read = errors.read(0); rejects([&] { short_read.get(); });
    auto backup = file.path(); backup += ".saved";
    std::filesystem::rename(file.path(), backup);
    auto failed_write = errors.write(0, page); rejects([&] { failed_write.get(); });
    std::filesystem::rename(backup, file.path());
  }
  { DiskScheduler idle(disk); } // Shutdown an empty sleeping worker.
}

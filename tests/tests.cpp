#include "buffer_pool.h"
#include "temp_file.h"
#include "check.h"
int main() {
  TempFile file(3); Disk disk(file.path());
  rejects([&] { BufferPool invalid(disk, 0); });
  BufferPool capacity(disk, 2);
  capacity.fetch(0); capacity.unpin(0); capacity.fetch(1); capacity.unpin(1);
  CHECK(capacity.resident() == 2); // Consume free frames before evicting residents.
  BufferPool pool(disk, 1);
  auto& first = pool.fetch(0);
  CHECK(&first == &pool.fetch(0)); CHECK(pool.resident() == 1); CHECK(pool.pins(0) == 2);
  rejects([&] { pool.fetch(1); });
  first.data[0] = 'A'; pool.unpin(0, true); pool.unpin(0);
  rejects([&] { pool.unpin(0); });
  rejects([&] { pool.fetch(99); }); CHECK(pool.resident() == 1);
  pool.fetch(1); pool.unpin(1); CHECK(disk.read(0)[0] == 'A');
  CHECK(pool.fetch(0).data[0] == 'A'); pool.unpin(0);
  pool.fetch(0).data[1] = 'B'; pool.unpin(0, true); pool.flush_all();
  CHECK(disk.read(0)[1] == 'B');
  std::filesystem::resize_file(file.path(), 1);
  rejects([&] { disk.read(0); });
  rejects([&] { Disk malformed(file.path()); });
  // A failed writeback must not evict or clear dirty state.
  TempFile other(2); Disk d2(other.path()); BufferPool p2(d2, 1);
  auto& dirty = p2.fetch(0); dirty.data[0] = 'Z'; p2.unpin(0, true);
  auto backup = other.path(); backup += ".backup";
  std::filesystem::rename(other.path(), backup);
  rejects([&] { p2.flush_all(); }); CHECK(dirty.dirty); CHECK(p2.resident() == 1);
  std::filesystem::rename(backup, other.path()); p2.flush_all(); CHECK(!dirty.dirty);
  CHECK(d2.read(0)[0] == 'Z');
}

#include "page_guard.h"
#include "temp_file.h"
#include "check.h"
#include <future>
#include <cstdlib>
#include <iostream>
#include <type_traits>
// A timeout must exit directly: an async future destructor would wait on the deadlock.
void flush_nested_regression(bool all, bool acquire_after_flush) {
  TempFile file(2); Disk disk(file.path()); BufferPool pool(disk, 2);
  { WritePageGuard dirty(pool, 0); dirty.mutable_data()[0] = 'D'; }
  {
    WritePageGuard outer(pool, 0);
    ReadPageGuard inner;
    if (!acquire_after_flush) inner = ReadPageGuard(pool, 1);
    auto flush = std::async(std::launch::async, [&] {
      try { if (all) pool.flush_all(); else pool.flush(0); }
      catch (const std::runtime_error&) { return true; }
      return false;
    });
    if (flush.wait_for(std::chrono::seconds(2)) != std::future_status::ready) {
      std::cerr << "flush blocked on nested guard\n";
      std::_Exit(2);
    }
    CHECK(flush.get());
    if (acquire_after_flush) inner = ReadPageGuard(pool, 1);
    inner.drop(); // Reverse-order cleanup must remain possible.
    CHECK(pool.pins(1) == 0);
    CHECK(disk.read(0)[0] == 0); // Rejected flush did not write or clear dirty.
  }
  CHECK(pool.pins(0) == 0);
  pool.flush_all(); CHECK(disk.read(0)[0] == 'D');
  { ReadPageGuard clean(pool, 0); rejects([&] { pool.flush(0); }); }
}
int main() {
  for (bool all : {false, true})
    for (bool acquire_after_flush : {false, true})
      flush_nested_regression(all, acquire_after_flush);
  static_assert(!std::is_copy_constructible_v<ReadPageGuard>);
  static_assert(std::is_nothrow_move_constructible_v<WritePageGuard>);
  TempFile file(3); Disk disk(file.path()); BufferPool pool(disk, 2);
  { ReadPageGuard first(pool, 0); }
  { ReadPageGuard second(pool, 1); }
  CHECK(pool.resident() == 2);
  ReadPageGuard empty; empty.drop(); rejects([&] { empty.data(); });
  {
    WritePageGuard a(pool, 0); a.mutable_data()[0] = 'G';
    WritePageGuard b(std::move(a)); CHECK(pool.pins(0) == 1); rejects([&] { a.data(); });
    WritePageGuard c(pool, 1); CHECK(pool.pins(1) == 1);
    c = std::move(b); CHECK(pool.pins(1) == 0); CHECK(pool.pins(0) == 1);
    auto& alias = c; c = std::move(alias); CHECK(pool.pins(0) == 1);
    c.drop(); c.drop(); CHECK(pool.pins(0) == 0);
  }
  pool.flush_all(); CHECK(disk.read(0)[0] == 'G');
  try { WritePageGuard write(pool, 0); write.mutable_data()[1] = 'E'; throw std::runtime_error("user"); }
  catch (const std::runtime_error&) {}
  CHECK(pool.pins(0) == 0); pool.flush_all(); CHECK(disk.read(0)[1] == 'E');
  rejects([&] { ReadPageGuard invalid(pool, 99); }); CHECK(pool.pins(0) == 0);
  BufferPool single(disk, 1);
  { ReadPageGuard read(single, 0); rejects([&] { ReadPageGuard full(single, 1); }); CHECK(single.pins(0) == 1); }
  { ReadPageGuard read(single, 1); } // Scope release made eviction possible.
  // Real shared readers: the second reader completes while the first still holds its latch.
  {
    ReadPageGuard read(pool, 0);
    auto second = std::async(std::launch::async, [&] { ReadPageGuard r(pool, 0); return r.data()[0]; });
    CHECK(second.get() == 'G'); CHECK(pool.pins(0) == 1);
    ReadPageGuard moved(std::move(read)); CHECK(pool.pins(0) == 1);
  }
  auto& frame = pool.fetch(0); pool.unpin(0);
  {
    WritePageGuard writer(pool, 0);
    auto probe = std::async(std::launch::async, [&] {
      bool shared = frame.latch.try_lock_shared(); if (shared) frame.latch.unlock_shared();
      bool exclusive = frame.latch.try_lock(); if (exclusive) frame.latch.unlock();
      return !shared && !exclusive;
    });
    CHECK(probe.get());
  }
  {
    ReadPageGuard reader(pool, 0);
    auto probe = std::async(std::launch::async, [&] {
      bool exclusive = frame.latch.try_lock(); if (exclusive) frame.latch.unlock(); return !exclusive;
    }); CHECK(probe.get());
  }
  // Deterministic start notification; blocked writer completes only after main drops reader.
  ReadPageGuard reader(pool, 0); std::promise<void> started;
  auto writer = std::async(std::launch::async, [&] {
    started.set_value(); WritePageGuard guard(pool, 0); guard.mutable_data()[2] = 'T';
  });
  started.get_future().get(); reader.drop(); writer.get();
  CHECK(pool.pins(0) == 0); pool.flush_all(); CHECK(disk.read(0)[2] == 'T');
  // Read-only guards must not dirty clean pages (otherwise flush attempts missing file).
  { ReadPageGuard read(pool, 0); CHECK(read.data()[0] == 'G'); }
  auto backup = file.path(); backup += ".backup";
  std::filesystem::rename(file.path(), backup); pool.flush_all();
  std::filesystem::rename(backup, file.path());
}

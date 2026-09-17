#include "work_queue.h"
#include "page_directory.h"
#include "check.h"
#include <atomic>
#include <thread>
#include <vector>
int main() {
  bool rejected = false;
  try { WorkQueue invalid(0); } catch (const std::invalid_argument &) { rejected = true; }
  CHECK(rejected);
  WorkQueue fifo(2);
  CHECK(fifo.push(7) && fifo.push(8)); fifo.close(); fifo.close();
  CHECK(!fifo.push(9) && fifo.pop() == 7 && fifo.pop() == 8 && !fifo.pop());
  // Do not throw before joining threads, even if the waiter handshake fails.
  WorkQueue empty(1);
  bool ended[2] = {false,false};
  std::thread e1([&] { ended[0] = !empty.pop(); });
  std::thread e2([&] { ended[1] = !empty.pop(); });
  bool waiting = empty.wait_for_waiters(0,2);
  empty.close(); e1.join(); e2.join(); CHECK(waiting && ended[0] && ended[1]);
  WorkQueue full(1); CHECK(full.push(1));
  bool refused[2] = {false,false};
  std::thread p1([&] { refused[0] = !full.push(2); });
  std::thread p2([&] { refused[1] = !full.push(3); });
  waiting = full.wait_for_waiters(2,0);
  full.close(); p1.join(); p2.join();
  CHECK(waiting && refused[0] && refused[1] && full.pop() == 1 && !full.pop());

  constexpr int producers = 4, consumers = 3, per_producer = 500;
  WorkQueue queue(3);
  std::vector<std::vector<int>> received(consumers);
  std::vector<std::thread> readers, writers;
  std::atomic<bool> accepted{true};
  for (int c = 0; c < consumers; ++c)
    readers.emplace_back([&,c] { while (auto page = queue.pop()) received[c].push_back(*page); });
  waiting = queue.wait_for_waiters(0, consumers);
  for (int p = 0; p < producers; ++p)
    writers.emplace_back([&,p] { for (int i = 0; i < per_producer; ++i)
      if (!queue.push(p*per_producer+i)) accepted = false; });
  for (auto &thread : writers) thread.join();
  queue.close(); for (auto &thread : readers) thread.join();
  CHECK(waiting && accepted);
  std::vector<int> counts(producers*per_producer, 0);
  for (const auto &batch : received) for (int id : batch) {
    CHECK(id >= 0 && id < producers*per_producer); ++counts[id];
  }
  for (int count : counts) CHECK(count == 1);

  PageDirectory directory;
  CHECK(!directory.get(0)); directory.set(0,0);
  WorkQueue start(1);
  std::atomic<bool> valid{true};
  std::vector<std::thread> workers;
  for (int w = 0; w < 2; ++w) workers.emplace_back([&,w] {
    start.pop(); for (int i = 0; i < 1000; ++i) directory.set(w, i);
  });
  for (int r = 0; r < 3; ++r) workers.emplace_back([&] {
    start.pop(); for (int i = 0; i < 1000; ++i) {
      auto value = directory.get(0); if (!value || *value < 0 || *value >= 1000) valid = false;
    }
  });
  waiting = start.wait_for_waiters(0,5); start.close();
  for (auto &thread : workers) thread.join();
  CHECK(waiting && valid && directory.get(0) == 999 && directory.get(1) == 999);
}

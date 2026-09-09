#include "work_queue.h"
#include "page_directory.h"
#include <iostream>
#include <thread>
int main() {
  WorkQueue queue(2);
  PageDirectory directory;
  std::thread consumer([&] { while (auto page = queue.pop()) directory.set(*page, *page * 100); });
  for (int page : {1,2,3}) queue.push(page);
  queue.close(); consumer.join();
  for (int page : {1,2,3}) std::cout << "page " << page << " offset " << *directory.get(page) << '\n';
  std::cout << "closed and drained: " << !queue.pop() << '\n';
}

#include "page_buffer.h"
#include "check.h"
#include <type_traits>
#include <vector>
int main() {
  static_assert(!std::is_copy_constructible_v<PageBuffer>);
  static_assert(std::is_nothrow_move_constructible_v<PageBuffer>);
  static_assert(std::is_nothrow_move_assignable_v<PageBuffer>);
  PageBuffer empty;
  CHECK(empty.size() == 0 && empty.data() == nullptr);
  bool rejected = false;
  try { empty.at(0); } catch (const std::out_of_range &) { rejected = true; }
  CHECK(rejected);
  PageBuffer a(4);
  for (std::size_t i = 0; i < a.size(); ++i) { CHECK(a.at(i) == 0); a.at(i) = static_cast<unsigned char>(i + 1); }
  const auto *address = a.data();
  PageBuffer b(std::move(a));
  CHECK(a.size() == 0 && !a.data() && b.data() == address);
  PageBuffer c(100);
  c = std::move(b);
  CHECK(!b.data() && b.size() == 0 && c.data() == address && c.size() == 4);
  const PageBuffer &view = c;
  CHECK(view.at(3) == 4);
  rejected = false;
  try { view.at(4); } catch (const std::out_of_range &) { rejected = true; }
  CHECK(rejected);
  auto &alias = c;
  c = std::move(alias); CHECK(c.data() == address && c.at(0) == 1);
  c = std::move(empty); CHECK(!c.data() && c.size() == 0);
  a = PageBuffer(2); CHECK(a.size() == 2 && a.at(0) == 0); // Reuse moved-from object.
  PageBuffer moved_empty(std::move(empty)); CHECK(!moved_empty.data() && moved_empty.size() == 0);
  std::vector<PageBuffer> pages;
  for (int i = 0; i < 1000; ++i) {
    PageBuffer p(1); p.at(0) = static_cast<unsigned char>(i % 256);
    pages.push_back(std::move(p)); CHECK(!p.data() && p.size() == 0);
  }
  for (std::size_t i = 0; i < pages.size(); ++i) CHECK(pages[i].at(0) == i % 256);
}

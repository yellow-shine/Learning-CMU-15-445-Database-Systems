#include "temp_directory.h"
#include "file_guard.h"
#include "check.h"
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <type_traits>
#include <unistd.h>

bool closed(int fd) { errno = 0; return ::fcntl(fd, F_GETFD) == -1 && errno == EBADF; }
int main() {
  static_assert(std::is_nothrow_destructible_v<FileGuard>);
  static_assert(!std::is_copy_constructible_v<FileGuard>);
  TempDirectory temp;
  auto path = (temp.path / "page").string();
  int fd = -1;
  { FileGuard file(path); fd = ::fileno(file.borrow()); file.write("page 7\n"); }
  CHECK(closed(fd));
  std::ifstream in(path);
  CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == "page 7\n");
  in.close();
  bool unwound = false;
  try {
    FileGuard file(path); fd = ::fileno(file.borrow()); file.write("partial");
    throw std::runtime_error("simulated executor failure");
  } catch (const std::runtime_error &) { unwound = true; }
  CHECK(unwound && closed(fd));
  bool failed = false;
  try { FileGuard file((temp.path / "missing" / "page").string()); }
  catch (const std::runtime_error &) { failed = true; }
  CHECK(failed);
  { FileGuard file(path); fd = ::fileno(file.borrow()); file.close(); file.close();
    CHECK(!file.borrow() && closed(fd));
    failed = false;
    try { file.write("x"); } catch (const std::logic_error &) { failed = true; }
    CHECK(failed);
  }
}

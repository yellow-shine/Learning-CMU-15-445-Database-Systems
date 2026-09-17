#include "tutorial.hpp"
#include <iostream>
using namespace tutorial;
void check(bool ok) { if (!ok) throw std::runtime_error("check failed"); }
template<class F> void rejects(F f) { bool caught=false; try { f(); } catch(const std::exception&) { caught=true; } check(caught); }
int main() { try {

LockManager m(false); m.begin(1); m.begin(2);
check(m.acquire(1,"x",Mode::Shared)); check(m.acquire(2,"x",Mode::Shared));
check(!m.acquire(1,"x",Mode::Exclusive));
m.unlock(2,"x"); check(m.acquire(1,"x",Mode::Exclusive));
check(m.acquire(1,"x",Mode::Shared));
rejects([&]{m.acquire(2,"y",Mode::Shared);});
m.unlock(1,"x"); rejects([&]{m.acquire(1,"z",Mode::Shared);});
m.finish(1); rejects([&]{m.acquire(1,"x",Mode::Shared);});
rejects([&]{m.begin(1);}); rejects([&]{m.acquire(9,"x",Mode::Shared);});
LockManager strict(true); strict.begin(1); strict.begin(2);
check(strict.acquire(1,"x",Mode::Exclusive));
check(!strict.acquire(2,"x",Mode::Shared)); check(!strict.acquire(2,"x",Mode::Exclusive));
rejects([&]{strict.unlock(1,"x");});
strict.finish(1); check(strict.acquire(2,"x",Mode::Exclusive)); strict.finish(2);
LockManager upgrade(true); upgrade.begin(1);
check(upgrade.acquire(1,"x",Mode::Shared)); check(upgrade.acquire(1,"y",Mode::Shared));
upgrade.unlock(1,"y"); rejects([&]{upgrade.acquire(1,"x",Mode::Exclusive);});
check(upgrade.acquire(1,"x",Mode::Shared)); upgrade.finish(1);
rejects([&]{upgrade.finish(1);});
LockManager empty(false); empty.begin(0); rejects([&]{empty.acquire(0,"",Mode::Shared);}); empty.finish(0);

std::cout << "all checks passed\n"; return 0;
} catch(const std::exception& e) { std::cerr << e.what() << "\n"; return 1; } }

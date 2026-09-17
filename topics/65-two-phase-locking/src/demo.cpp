#include "tutorial.hpp"
#include <iostream>
using namespace tutorial;
int main() {

LockManager locks(true); locks.begin(1); locks.begin(2);
locks.acquire(1,"row",Mode::Shared); locks.acquire(2,"row",Mode::Shared);
std::cout << "upgrade with reader: " << locks.acquire(1,"row",Mode::Exclusive) << '\n';
locks.finish(2);
std::cout << "upgrade after release: " << locks.acquire(1,"row",Mode::Exclusive) << '\n';
locks.finish(1);

}

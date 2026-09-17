#include "tutorial.hpp"
#include <iostream>
using namespace tutorial;
int main() {

MVCC db({{"balance",100}}); int old=db.begin(), writer=db.begin();
db.write(writer,"balance",80);
std::cout << "before commit: " << *db.read(old,"balance") << '\n'; db.commit(writer);
int fresh=db.begin();
std::cout << "old snapshot: " << *db.read(old,"balance") << '\n';
std::cout << "new snapshot: " << *db.read(fresh,"balance") << '\n'; db.commit(old); db.commit(fresh);

}

#include "tutorial.hpp"
#include <iostream>
using namespace tutorial;
int main() {

TimestampOrdering db({{"x",10}}); auto t1=db.begin(), t2=db.begin();
std::cout << "young reads: " << *db.read(t2,"x") << '\n';
std::cout << "old write accepted: " << db.write(t1,"x",20) << '\n';
std::cout << "young commits: " << db.commit(t2) << '\n';

}

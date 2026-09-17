#include "tutorial.hpp"
#include <iostream>
using namespace tutorial;
int main() {

MVCC db({{"x",0}}); int a=db.begin(), b=db.begin();
db.write(a,"x",1); db.write(b,"x",2);
std::cout << "A commits: " << db.commit(a) << '\n';
std::cout << "B still reads own: " << *db.read(b,"x") << '\n';
std::cout << "B commits: " << db.commit(b) << '\n';
int c=db.begin(); std::cout << "published x: " << *db.read(c,"x") << '\n'; db.commit(c);

}

#include "tutorial.hpp"
#include <iostream>
using namespace tutorial;
int main() {

OCC db({{"counter",0}}); int a=db.begin(), b=db.begin();
db.write(a,"counter",db.read(a,"counter")+1); db.write(b,"counter",db.read(b,"counter")+1);
std::cout << "commit A: " << db.commit(a) << '\n' << "commit B: " << db.commit(b) << '\n';
int c=db.begin(); std::cout << "counter: " << db.read(c,"counter") << '\n'; db.commit(c);

}

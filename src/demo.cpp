#include "tutorial.hpp"
#include <iostream>
using namespace tutorial;
int main() {

MVCC doctors({{"alice",1},{"bob",1}});int a=doctors.begin(),b=doctors.begin();
if(doctors.read(a,"bob")==1)doctors.write(a,"alice",0);
if(doctors.read(b,"alice")==1)doctors.write(b,"bob",0);
std::cout << "A commits: " << doctors.commit(a) << '\n';
std::cout << "B commits: " << doctors.commit(b) << '\n';
int c=doctors.begin();std::cout << "on call: " << *doctors.read(c,"alice")+*doctors.read(c,"bob") << '\n';doctors.commit(c);

}

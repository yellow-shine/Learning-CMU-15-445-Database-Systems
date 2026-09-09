#include "tutorial.hpp"
#include <iostream>
using namespace tutorial;
int main() {

Deadlocks d; d.begin(1); d.begin(2);
d.request(1,"a"); d.request(2,"b"); d.request(1,"b"); d.request(2,"a");
std::cout << "cycle size: " << d.cycle().size() << '\n';
std::cout << "victim: " << *d.resolve_one() << '\n';
std::cout << "retry T1: " << d.request(1,"b") << '\n'; d.commit(1);

}

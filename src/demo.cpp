#include "tutorial.hpp"
#include <iostream>
using namespace tutorial;
int main() {

Bank bank({100,50});
{ Transaction t(bank); t.transfer(0,1,30); t.commit(); }
std::cout << "committed: " << bank.balance(0) << ' ' << bank.balance(1) << '\n';
{ Transaction t(bank); t.transfer(0,1,10); t.abort(); }
std::cout << "aborted: " << bank.balance(0) << ' ' << bank.balance(1) << '\n';

}

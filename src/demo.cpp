#include "tutorial.hpp"
#include <iostream>
using namespace tutorial;
int main() {

for(const auto& a:anomalies()) std::cout << a.name << ": " << a.first << " -> " << a.second << '\n';

}

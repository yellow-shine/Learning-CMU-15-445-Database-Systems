#include "tutorial.hpp"
#include <iostream>
using namespace tutorial;
int main() {

Schedule s={{1,"x",Kind::Read},{2,"x",Kind::Write},{2,"y",Kind::Read},{1,"y",Kind::Write}};
auto graph=precedence(s);
for (const auto& [v,edges]:graph) for(int w:edges) std::cout << v << " -> " << w << '\n';
std::cout << "serializable: " << serial_order(graph).has_value() << '\n';

}

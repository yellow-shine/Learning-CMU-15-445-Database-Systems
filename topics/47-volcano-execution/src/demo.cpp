#include "engine.h"
#include <iostream>
int main() {

Scan scan({{1,5},{2,20},{3,10},{2,20}}); Filter filter(scan,10); Projection project(filter);
project.Init(); int key;
while (project.Next(key)) std::cout << key << ' ';
std::cout << "\nscan calls=" << scan.calls << " reads=" << scan.reads
          << " filter calls=" << filter.calls << " projection calls=" << project.calls << '\n';

}

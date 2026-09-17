#include "engine.h"
#include <iostream>
int main() {

Scan scan({0,1,2,3,4,5}); Limit query(scan,2,2); query.Init();
int value; while(query.Next(value)) std::cout << value << ' ';
std::cout << "\nreads=" << scan.reads << " calls=" << scan.calls << '\n';

}

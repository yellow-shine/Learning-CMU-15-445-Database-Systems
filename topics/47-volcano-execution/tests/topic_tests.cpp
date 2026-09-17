#include "engine.h"
#include <iostream>
#include <stdexcept>
#define CHECK(...) do { if (!(__VA_ARGS__)) throw std::runtime_error("check failed: " #__VA_ARGS__); } while(false)
int main() { try {

Scan scan({{1,5},{2,20},{3,10},{2,20}}); Filter filter(scan,10); Projection project(filter);
project.Init(); CHECK(scan.reads == 0);
int key = -1; CHECK(project.Next(key) && key == 2); CHECK(scan.reads == 2);
CHECK(project.Next(key) && key == 3); CHECK(project.Next(key) && key == 2);
CHECK(!project.Next(key)); auto calls = scan.calls;
CHECK(!project.Next(key) && key == 2 && scan.calls == calls);
project.Init(); CHECK(project.Next(key) && key == 2 && scan.reads == 2);
Scan empty({}); Filter ef(empty,0); Projection ep(ef); ep.Init(); CHECK(!ep.Next(key)); CHECK(!ep.Next(key));
Scan rejected({{1,-1}}); Filter rf(rejected,0); Projection rp(rf); rp.Init(); CHECK(!rp.Next(key)); CHECK(rejected.reads==1);

std::cout << "all checks passed\n";
} catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

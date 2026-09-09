#include "vector_search.h"
#include <iostream>
int main() {
    vectors::ExactIndex index(2);
    index.add({1,0}); index.add({-1,0}); index.add({0,2});
    for(auto hit:index.search({0,0},2)) std::cout << "id=" << hit.id << " squared_l2=" << hit.distance << '\n';
}

#include "bplus_tree.h"
#include <iostream>
#include <thread>
int main() {
    ConcurrentBPlusTree tree;
    std::thread a([&]{for(int i=0;i<100;++i) tree.put(i,i);});
    std::thread b([&]{for(int i=100;i<200;++i) tree.put(i,i);});
    a.join(); b.join(); tree.validate();
    std::cout << "key42=" << *tree.get(42) << " key142=" << *tree.get(142) << '\n';
}

#include "bplus_tree.h"
#include <iostream>
int main() {
    BPlusTree tree;
    for(int key=1;key<=15;++key) tree.put(key,key*10);
    tree.validate();
    std::cout << "height=" << tree.height() << " key7=" << *tree.get(7) << '\n';
}

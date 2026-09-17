#include "bplus_tree.h"
#include <iostream>
int main() {
    BPlusTree tree;
    for(int key=1;key<=15;++key) tree.put(key,key*10);
    std::cout << "before=" << tree.height();
    for(int key=1;key<=15;++key) { tree.erase(key); tree.validate(); }
    std::cout << " after=" << tree.height() << " missing=" << !tree.get(7) << '\n';
}

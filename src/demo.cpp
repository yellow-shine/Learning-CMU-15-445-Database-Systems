#include "bplus_tree.h"
#include <iostream>
int main() {
    BPlusTree tree;
    for(int key=1;key<=15;++key) tree.put(key,key*10);
    tree.erase(7); tree.validate();
    for(auto [key,value]:tree.range(5,10)) std::cout << key << ':' << value << ' ';
    std::cout << '\n';
}

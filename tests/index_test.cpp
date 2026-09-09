#include "bplus_tree.h"
#include <climits>
#include <iostream>
#include <map>
#include <random>
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (false)
int main() {
    BPlusTree tree; std::map<int,int> oracle;
    tree.validate(); CHECK(!tree.get(0));
    std::mt19937 rng(33);
    for (int i=0;i<4000;++i) {
        int key=static_cast<int>(rng()%1000)-500;
        tree.put(key,i); oracle[key]=i; tree.validate();
        for (int probe : {key,-501,501}) {
            auto it=oracle.find(probe);
            CHECK(tree.get(probe)==(it==oracle.end()?std::optional<int>{}:it->second));
        }
    }
    for (auto [k,v]:oracle) CHECK(tree.get(k)==v);
    tree.put(INT_MIN,1); tree.put(INT_MAX,2); tree.validate();
    CHECK(tree.get(INT_MIN)==1 && tree.get(INT_MAX)==2); CHECK(tree.height()>2);
    for (int direction : {-1,1}) {
        BPlusTree ordered;
        for (int i=0;i<1000;++i) { ordered.put(direction*i,i); ordered.validate(); }
        for (int i=0;i<1000;++i) CHECK(ordered.get(direction*i)==i);
    }
    std::cout << "insert: differential and structural checks passed\n";
}

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
    BPlusTree mixed; std::map<int,int> expected;
    for (int i=0;i<16000;++i) {
        int key=static_cast<int>(rng()%400);
        if (rng()%2) { mixed.put(key,i); expected[key]=i; }
        else CHECK(mixed.erase(key)==(expected.erase(key)!=0));
        mixed.validate();
        for (int k=0;k<400;++k) {
            auto it=expected.find(k);
            CHECK(mixed.get(k)==(it==expected.end()?std::optional<int>{}:it->second));
        }
    }
    for (int k=0;k<400;++k) { mixed.erase(k); mixed.validate(); }
    CHECK(mixed.height()==1); CHECK(!mixed.erase(9));
    const auto& c=mixed.rebalance_counts();
    CHECK(c.borrow_left && c.borrow_right && c.merge_left && c.merge_right);
    CHECK(c.internal_merge && c.root_shrink);
    for (int direction : {-1,1}) {
        BPlusTree drain;
        for (int i=0;i<700;++i) drain.put(i,i);
        for (int i=0;i<700;++i) {
            CHECK(drain.erase(direction==1?i:699-i)); drain.validate();
        }
        CHECK(drain.height()==1);
    }
    std::cout << "rebalance left/right borrow+merge, recursive merge, shrink passed\n";
    std::cout << "insert: differential and structural checks passed\n";
}

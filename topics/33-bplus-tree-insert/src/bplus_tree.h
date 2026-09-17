#pragma once
#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

// Integer unique keys, upsert values. Maximum four keys / five children.
class BPlusTree {
    struct Node {
        bool leaf;
        std::vector<int> keys, values;
        std::vector<std::unique_ptr<Node>> children;
        Node* next = nullptr;
        explicit Node(bool is_leaf) : leaf(is_leaf) {}
    };
    std::unique_ptr<Node> root_ = std::make_unique<Node>(true);
    static int minimum(const Node* n) {
        while (!n->leaf) n = n->children.front().get();
        return n->keys.front();
    }
    static void separators(Node* n) {
        if (n->leaf) return;
        n->keys.clear();
        for (size_t i = 1; i < n->children.size(); ++i)
            n->keys.push_back(minimum(n->children[i].get()));
    }
    static size_t route(const Node* n, int key) {
        return std::upper_bound(n->keys.begin(), n->keys.end(), key) - n->keys.begin();
    }
    static std::unique_ptr<Node> insert(Node* n, int key, int value) {
        if (n->leaf) {
            auto it = std::lower_bound(n->keys.begin(), n->keys.end(), key);
            size_t i = it - n->keys.begin();
            if (it != n->keys.end() && *it == key) { n->values[i] = value; return {}; }
            n->keys.insert(it, key);
            n->values.insert(n->values.begin() + i, value);
        } else {
            size_t i = route(n, key);
            auto right = insert(n->children[i].get(), key, value);
            if (right) n->children.insert(n->children.begin() + i + 1, std::move(right));
            separators(n);
        }
        if (n->keys.size() <= 4) return {};
        auto right = std::make_unique<Node>(n->leaf);
        if (n->leaf) {
            right->keys.assign(n->keys.begin() + 2, n->keys.end());
            right->values.assign(n->values.begin() + 2, n->values.end());
            n->keys.resize(2); n->values.resize(2);
            right->next = n->next; n->next = right.get();
        } else {
            for (size_t i = 3; i < n->children.size(); ++i)
                right->children.push_back(std::move(n->children[i]));
            n->children.resize(3);
            separators(n); separators(right.get());
        }
        return right;
    }
    static void require(bool ok) { if (!ok) throw std::logic_error("B+Tree invariant"); }
    static std::pair<int,int> check(const Node* n, bool root, size_t depth,
                                   size_t& leaf_depth, std::vector<const Node*>& leaves) {
        require(std::is_sorted(n->keys.begin(), n->keys.end()));
        require(std::adjacent_find(n->keys.begin(), n->keys.end()) == n->keys.end());
        require(n->keys.size() <= 4);
        if (n->leaf) {
            require(n->keys.size() == n->values.size() && n->children.empty());
            require(root || n->keys.size() >= 2);
            if (leaves.empty()) leaf_depth = depth;
            require(depth == leaf_depth); leaves.push_back(n);
            if (n->keys.empty()) { require(root); return {0,0}; }
            return {n->keys.front(), n->keys.back()};
        }
        require(n->children.size() == n->keys.size() + 1 && n->values.empty());
        require(n->children.size() >= (root ? 2U : 3U));
        auto bounds = check(n->children[0].get(), false, depth+1, leaf_depth, leaves);
        for (size_t i = 1; i < n->children.size(); ++i) {
            auto b = check(n->children[i].get(), false, depth+1, leaf_depth, leaves);
            require(bounds.second < b.first && n->keys[i-1] == b.first);
            bounds.second = b.second;
        }
        return bounds;
    }
public:
    void put(int key, int value) {
        auto right = insert(root_.get(), key, value);
        if (right) {
            auto root = std::make_unique<Node>(false);
            root->children.push_back(std::move(root_));
            root->children.push_back(std::move(right));
            separators(root.get()); root_ = std::move(root);
        }
    }
    std::optional<int> get(int key) const {
        const Node* n = root_.get();
        while (!n->leaf) n = n->children[route(n,key)].get();
        auto it = std::lower_bound(n->keys.begin(), n->keys.end(), key);
        if (it == n->keys.end() || *it != key) return {};
        return n->values[it-n->keys.begin()];
    }
    size_t height() const {
        size_t h=1; const Node* n=root_.get();
        while (!n->leaf) { ++h; n=n->children[0].get(); }
        return h;
    }
    void validate() const {
        std::vector<const Node*> leaves; size_t depth=0;
        check(root_.get(), true, 0, depth, leaves);
        for (size_t i=0; i<leaves.size(); ++i)
            require(leaves[i]->next == (i+1<leaves.size() ? leaves[i+1] : nullptr));
    }
};

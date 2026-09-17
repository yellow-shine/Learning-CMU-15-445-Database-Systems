#pragma once
#include <algorithm>
#include <atomic>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

// Integer unique keys, upsert values. Maximum four keys / five children.
class ConcurrentBPlusTree {
    struct Node {
        mutable std::shared_mutex latch;
        bool leaf;
        std::vector<int> keys, values;
        std::vector<std::shared_ptr<Node>> children;
        Node* next = nullptr;
        explicit Node(bool is_leaf) : leaf(is_leaf) {}
    };
    std::shared_ptr<Node> root_ = std::make_shared<Node>(true);
    mutable std::shared_mutex root_gate_;
    std::atomic<size_t> fast_writes_{0};
    static int minimum(const Node* n) {
        std::shared_lock<std::shared_mutex> lock(n->latch);
        if (n->leaf) return n->keys.front();
        return minimum(n->children.front().get());
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
    static std::shared_ptr<Node> insert(Node* n, int key, int value) {
        std::unique_lock<std::shared_mutex> lock(n->latch);
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
        auto right = std::make_shared<Node>(n->leaf);
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
    // Count actual rebalancing paths, so tests prove both directions execute.
    struct RebalanceCounts {
        size_t borrow_left=0, borrow_right=0, merge_left=0, merge_right=0;
        size_t internal_merge=0, root_shrink=0;
    } counts_;
    static size_t occupancy(const Node* n) {
        return n->leaf ? n->keys.size() : n->children.size();
    }
    static size_t minimum_occupancy(const Node* n) { return n->leaf ? 2 : 3; }
    static void move_last_to_front(Node* left, Node* right) {
        if (left->leaf) {
            right->keys.insert(right->keys.begin(),left->keys.back());
            right->values.insert(right->values.begin(),left->values.back());
            left->keys.pop_back(); left->values.pop_back();
        } else {
            right->children.insert(right->children.begin(),std::move(left->children.back()));
            left->children.pop_back(); separators(left); separators(right);
        }
    }
    static void move_first_to_back(Node* left, Node* right) {
        if (left->leaf) {
            left->keys.push_back(right->keys.front());
            left->values.push_back(right->values.front());
            right->keys.erase(right->keys.begin()); right->values.erase(right->values.begin());
        } else {
            left->children.push_back(std::move(right->children.front()));
            right->children.erase(right->children.begin()); separators(left); separators(right);
        }
    }
    void merge(Node* parent, size_t left_index) {
        Node* left=parent->children[left_index].get();
        Node* right=parent->children[left_index+1].get();
        if (left->leaf) {
            left->keys.insert(left->keys.end(),right->keys.begin(),right->keys.end());
            left->values.insert(left->values.end(),right->values.begin(),right->values.end());
            left->next=right->next;
        } else {
            ++counts_.internal_merge;
            for (auto& child:right->children) left->children.push_back(std::move(child));
            separators(left);
        }
        parent->children.erase(parent->children.begin()+left_index+1);
    }
    bool erase(Node* n, int key) {
        std::unique_lock<std::shared_mutex> lock(n->latch);
        if (n->leaf) {
            auto it=std::lower_bound(n->keys.begin(),n->keys.end(),key);
            if (it==n->keys.end() || *it!=key) return false;
            n->values.erase(n->values.begin()+(it-n->keys.begin())); n->keys.erase(it);
            return true;
        }
        size_t i=route(n,key);
        if (!erase(n->children[i].get(),key)) return false;
        // Parent is exclusive: sibling lock order cannot race another rebalance.
        auto held_children=n->children; // keep detached node mutexes alive until unlock
        std::vector<std::unique_lock<std::shared_mutex>> sibling_locks;
        for (const auto& c:held_children) sibling_locks.emplace_back(c->latch);
        Node* child=n->children[i].get();
        if (occupancy(child)<minimum_occupancy(child)) {
            if (i>0 && occupancy(n->children[i-1].get())>minimum_occupancy(child)) {
                move_last_to_front(n->children[i-1].get(),child); ++counts_.borrow_left;
            } else if (i+1<n->children.size() && occupancy(n->children[i+1].get())>minimum_occupancy(child)) {
                move_first_to_back(child,n->children[i+1].get()); ++counts_.borrow_right;
            } else if (i>0) { merge(n,i-1); ++counts_.merge_left; }
            else { merge(n,i); ++counts_.merge_right; }
        }
        sibling_locks.clear();
        separators(n); return true;
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
    // Shared internal coupling, then exclusive leaf; unsafe writes restart.
    bool try_leaf(int key, int value, bool deleting, bool& result,
                  const std::function<void()>& hook) {
        std::shared_lock<std::shared_mutex> gate(root_gate_);
        auto n=root_;
        std::shared_lock<std::shared_mutex> parent(n->latch);
        gate.unlock();
        if (n->leaf) return false;
        while (true) {
            auto child=n->children[route(n.get(),key)];
            if (!child->leaf) {
                std::shared_lock<std::shared_mutex> next(child->latch);
                parent.unlock(); n=std::move(child); parent=std::move(next); continue;
            }
            std::unique_lock<std::shared_mutex> leaf(child->latch);
            parent.unlock();
            auto it=std::lower_bound(child->keys.begin(),child->keys.end(),key);
            size_t i=it-child->keys.begin();
            bool exists=it!=child->keys.end() && *it==key;
            if (deleting) {
                if (!exists) { result=false; return true; }
                if (i==0 || child->keys.size()<=2) return false;
            } else if (!exists && (child->keys.size()>=4 || i==0)) return false;
            if (hook) hook(); // test seam: called with ONLY this leaf exclusively held
            if (deleting) {
                child->keys.erase(it); child->values.erase(child->values.begin()+i);
            } else if (exists) child->values[i]=value;
            else { child->keys.insert(it,key); child->values.insert(child->values.begin()+i,value); }
            ++fast_writes_; result=true; return true;
        }
    }
public:
    size_t fast_writes() const { return fast_writes_.load(); }
    bool erase(int key) {
        bool result=false;
        if (try_leaf(key,0,true,result,{})) return result;
        // ponytail: structural changes retain ancestors; optimistic safe leaf writes
        // run in parallel. Top-down structural splitting is the next throughput upgrade.
        std::unique_lock<std::shared_mutex> gate(root_gate_);
        bool found=erase(root_.get(),key);
        if (!root_->leaf && root_->children.size()==1) root_=root_->children.front();
        return found;
    }
    void put(int key, int value, const std::function<void()>& leaf_hook={}) {
        bool result=false;
        if (try_leaf(key,value,false,result,leaf_hook)) return;
        std::unique_lock<std::shared_mutex> gate(root_gate_);
        auto right=insert(root_.get(),key,value);
        if (right) {
            auto root=std::make_shared<Node>(false);
            root->children.push_back(root_); root->children.push_back(right);
            separators(root.get()); root_=std::move(root);
        }
    }
    std::optional<int> get(int key) const {
        std::shared_lock<std::shared_mutex> gate(root_gate_);
        auto n=root_;
        std::shared_lock<std::shared_mutex> lock(n->latch);
        gate.unlock();
        while (!n->leaf) {
            auto child=n->children[route(n.get(),key)];
            std::shared_lock<std::shared_mutex> next(child->latch);
            lock.unlock(); n=std::move(child); lock=std::move(next);
        }
        auto it=std::lower_bound(n->keys.begin(),n->keys.end(),key);
        if (it==n->keys.end() || *it!=key) return {};
        return n->values[it-n->keys.begin()];
    }
    // Introspection is quiescent-only (call after joining all workers).
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

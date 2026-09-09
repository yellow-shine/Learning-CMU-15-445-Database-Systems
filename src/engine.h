#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>
struct Row {
    int key; int value;
    bool operator==(const Row& other) const { return key==other.key && value==other.value; }
};
enum class FailPoint { None, AfterTable, DuringIndex };
class ModificationTable {
public:
    using RID = std::size_t;
    using Rows = std::vector<std::optional<Row>>;
private:
    Rows rows_;
    std::map<int,RID> index_; // Auxiliary unique index, not a B+Tree implementation.
    void Publish(Rows candidate, FailPoint fail) {
        if(fail==FailPoint::AfterTable) throw std::runtime_error("injected after table edits");
        std::map<int,RID> index;
        for(RID rid=0; rid<candidate.size(); ++rid) if(candidate[rid]) {
            if(!index.emplace(candidate[rid]->key,rid).second) throw std::invalid_argument("duplicate key");
            if(fail==FailPoint::DuringIndex) throw std::runtime_error("injected after index insertion");
        }
        // Both swaps are noexcept with these standard allocators: publication cannot be partial.
        rows_.swap(candidate); index_.swap(index);
    }
    static void CheckRID(const Rows& rows, RID rid, std::set<RID>& seen) {
        if(rid>=rows.size() || !rows[rid]) throw std::out_of_range("missing RID");
        if(!seen.insert(rid).second) throw std::invalid_argument("duplicate target RID");
    }
public:
    const Rows& rows() const { return rows_; }
    std::optional<RID> Find(int key) const {
        auto found=index_.find(key);
        if(found==index_.end()) return std::nullopt;
        return found->second;
    }
    // ponytail: copy-on-write O(n) statements; use undo logging for large tables.
    std::vector<RID> Insert(const std::vector<Row>& input, FailPoint fail=FailPoint::None) {
        auto candidate=rows_;
        std::vector<RID> added;
        for(auto row:input) { added.push_back(candidate.size()); candidate.push_back(row); }
        Publish(std::move(candidate),fail); return added;
    }
    void Update(const std::vector<std::pair<RID,Row>>& updates, FailPoint fail=FailPoint::None) {
        auto candidate=rows_; std::set<RID> seen;
        for(const auto& update:updates) {
            CheckRID(candidate,update.first,seen); candidate[update.first]=update.second;
        }
        Publish(std::move(candidate),fail);
    }
    void Delete(const std::vector<RID>& targets, FailPoint fail=FailPoint::None) {
        auto candidate=rows_; std::set<RID> seen;
        for(auto rid:targets) { CheckRID(candidate,rid,seen); candidate[rid].reset(); }
        Publish(std::move(candidate),fail);
    }
    bool Consistent() const {
        std::size_t live=0;
        for(RID rid=0; rid<rows_.size(); ++rid) if(rows_[rid]) {
            ++live;
            if(Find(rows_[rid]->key)!=std::optional<RID>(rid)) return false;
        }
        return live==index_.size();
    }
};

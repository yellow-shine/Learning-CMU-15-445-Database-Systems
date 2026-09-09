#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>
struct Row { int key; int value; };
struct AccessResult {
    std::vector<std::size_t> rids;
    std::size_t heap_reads=0, index_entries=0, seek_comparisons=0;
};
class AccessTable {
    struct Entry { int key; std::size_t rid; };
    std::vector<Row> rows_;
    std::vector<Entry> index_;
public:
    explicit AccessTable(std::vector<Row> rows) : rows_(std::move(rows)) {
        for(std::size_t rid=0; rid<rows_.size(); ++rid) index_.push_back({rows_[rid].key,rid});
        std::sort(index_.begin(),index_.end(),[](Entry a, Entry b) {
            return a.key < b.key || (a.key==b.key && a.rid<b.rid);
        });
    }
    const Row& row(std::size_t rid) const { return rows_.at(rid); }
    AccessResult SeqScan(int low, int high, int minimum) const {
        AccessResult result;
        if(low>high) return result;
        for(std::size_t rid=0; rid<rows_.size(); ++rid) {
            ++result.heap_reads;
            auto row=rows_[rid];
            if(row.key>=low && row.key<=high && row.value>=minimum) result.rids.push_back(rid);
        }
        return result;
    }
    AccessResult IndexScan(int low, int high, int minimum) const {
        AccessResult result;
        if(low>high) return result;
        std::size_t left=0, right=index_.size();
        while(left<right) {
            auto middle=left+(right-left)/2; ++result.seek_comparisons;
            if(index_[middle].key<low) left=middle+1; else right=middle;
        }
        for(auto i=left; i<index_.size() && index_[i].key<=high; ++i) {
            ++result.index_entries; ++result.heap_reads;
            const auto rid=index_[i].rid;
            if(rows_[rid].value>=minimum) result.rids.push_back(rid);
        }
        return result;
    }
};

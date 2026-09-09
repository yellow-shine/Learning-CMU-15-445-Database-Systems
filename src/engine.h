#pragma once

#include <cstddef>
#include <utility>
#include <vector>
struct Row { int key; int value; };
struct Stats { std::size_t scan_calls=0, filter_calls=0, projection_calls=0, reads=0; };
inline std::vector<Row> Scan(const std::vector<Row>& table, Stats& stats) {
    ++stats.scan_calls; stats.reads += table.size(); return table;
}
inline std::vector<Row> Filter(const std::vector<Row>& input, int minimum, Stats& stats) {
    ++stats.filter_calls;
    std::vector<Row> output;
    for (auto row : input) if (row.value >= minimum) output.push_back(row);
    return output;
}
inline std::vector<int> Projection(const std::vector<Row>& input, Stats& stats) {
    ++stats.projection_calls;
    std::vector<int> output;
    for (auto row : input) output.push_back(row.key);
    return output;
}
class MaterializedQuery {
    std::vector<Row> table_, scanned_, filtered_;
    std::vector<int> projected_;
    int minimum_;
    std::size_t pos_ = 0;
public:
    Stats stats;
    MaterializedQuery(std::vector<Row> table, int minimum) : table_(std::move(table)), minimum_(minimum) {}
    void Init() {
        pos_ = 0; stats = {};
        scanned_ = Scan(table_, stats);
        filtered_ = Filter(scanned_, minimum_, stats);
        projected_ = Projection(filtered_, stats);
    }
    bool Next(int& key) {
        if (pos_ == projected_.size()) return false;
        key = projected_[pos_++]; return true;
    }
    // Logical retained payload, not allocator capacity or the input table.
    std::size_t intermediate_bytes() const {
        return scanned_.size()*sizeof(Row) + filtered_.size()*sizeof(Row) + projected_.size()*sizeof(int);
    }
    std::size_t result_size() const { return projected_.size(); }
};

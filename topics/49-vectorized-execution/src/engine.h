#pragma once

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>
struct Row { int key; int value; };
struct Batch { std::vector<Row> rows; std::vector<std::size_t> selection; };
class BatchScan {
    std::vector<Row> table_;
    std::size_t width_, pos_=0;
public:
    std::size_t calls=0, reads=0;
    BatchScan(std::vector<Row> table, std::size_t width) : table_(std::move(table)), width_(width) {
        if (width==0) throw std::invalid_argument("batch width must be positive");
    }
    void Init() { pos_=calls=reads=0; }
    bool Next(Batch& batch) {
        ++calls; batch.rows.clear(); batch.selection.clear();
        auto count = std::min(width_, table_.size()-pos_);
        for (std::size_t i=0; i<count; ++i) {
            batch.rows.push_back(table_[pos_++]); batch.selection.push_back(i); ++reads;
        }
        return count!=0;
    }
};
inline void Filter(Batch& batch, int minimum) {
    auto& selected = batch.selection;
    selected.erase(std::remove_if(selected.begin(), selected.end(), [&](std::size_t i) {
        return batch.rows.at(i).value < minimum;
    }), selected.end());
}
inline std::vector<int> Projection(const Batch& batch) {
    std::vector<int> keys;
    for (auto i : batch.selection) keys.push_back(batch.rows.at(i).key);
    return keys;
}
class VectorizedQuery {
    BatchScan scan_;
    int minimum_;
    Batch batch_;
    bool done_=false;
public:
    std::size_t filter_calls=0, projection_calls=0;
    VectorizedQuery(std::vector<Row> table, std::size_t width, int minimum)
        : scan_(std::move(table),width), minimum_(minimum) {}
    void Init() { scan_.Init(); done_=false; filter_calls=projection_calls=0; }
    bool Next(std::vector<int>& keys) {
        keys.clear();
        if (done_) return false;
        while (scan_.Next(batch_)) {
            ++filter_calls; Filter(batch_,minimum_);
            ++projection_calls; keys=Projection(batch_);
            if (!keys.empty()) return true;
        }
        done_=true; return false;
    }
    const BatchScan& scan() const { return scan_; }
};

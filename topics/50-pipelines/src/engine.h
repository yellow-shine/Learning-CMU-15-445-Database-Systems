#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>
struct Row { int key; int value; };
class PipelineQuery {
    std::vector<Row> table_, sorted_;
    int minimum_;
    std::size_t pos_=0;
    bool built_=false, done_=false;
    void Build() {
        trace.push_back("P1.begin");
        for (auto row : table_) {
            ++reads; trace.push_back("scan:"+std::to_string(row.key));
            if (row.value < minimum_) {
                trace.push_back("filter.drop:"+std::to_string(row.key)); continue;
            }
            trace.push_back("filter.pass:"+std::to_string(row.key));
            sorted_.push_back(row);
            trace.push_back("sort.sink:"+std::to_string(row.key));
        }
        trace.push_back("P1.end");
        std::stable_sort(sorted_.begin(),sorted_.end(),[](Row a, Row b) { return a.value<b.value; });
        trace.push_back("sort.ready"); built_=true;
        trace.push_back("P2.begin");
    }
public:
    std::vector<std::string> trace;
    std::size_t reads=0;
    PipelineQuery(std::vector<Row> table, int minimum) : table_(std::move(table)), minimum_(minimum) {}
    void Init() { sorted_.clear(); trace.clear(); pos_=reads=0; built_=done_=false; }
    bool Next(int& key) {
        if (done_) return false;
        if (!built_) Build();
        if (pos_==sorted_.size()) { trace.push_back("P2.end"); done_=true; return false; }
        key=sorted_[pos_++].key;
        trace.push_back("sort.source:"+std::to_string(key));
        trace.push_back("project:"+std::to_string(key)); return true;
    }
    std::size_t buffered_rows() const { return sorted_.size(); }
};

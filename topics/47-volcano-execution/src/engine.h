#pragma once

#include <cstddef>
#include <utility>
#include <vector>
struct Row { int key; int value; };
struct Iterator {
    virtual ~Iterator() = default;
    virtual void Init() = 0;
    virtual bool Next(Row&) = 0;
};
class Scan final : public Iterator {
    std::vector<Row> rows_;
    std::size_t pos_ = 0;
public:
    std::size_t calls = 0, reads = 0;
    explicit Scan(std::vector<Row> rows) : rows_(std::move(rows)) {}
    void Init() override { pos_ = calls = reads = 0; }
    bool Next(Row& out) override {
        ++calls;
        if (pos_ == rows_.size()) return false;
        out = rows_[pos_++]; ++reads; return true;
    }
};
class Filter final : public Iterator {
    Iterator& child_;
    int minimum_;
    bool done_ = false;
public:
    std::size_t calls = 0;
    Filter(Iterator& child, int minimum) : child_(child), minimum_(minimum) {}
    void Init() override { child_.Init(); done_ = false; calls = 0; }
    bool Next(Row& out) override {
        ++calls;
        if (done_) return false;
        Row row;
        while (child_.Next(row)) {
            if (row.value >= minimum_) { out = row; return true; }
        }
        done_ = true; return false;
    }
};
class Projection {
    Iterator& child_;
    bool done_ = false;
public:
    std::size_t calls = 0;
    explicit Projection(Iterator& child) : child_(child) {}
    void Init() { child_.Init(); done_ = false; calls = 0; }
    bool Next(int& key) {
        ++calls;
        if (done_) return false;
        Row row;
        if (!child_.Next(row)) { done_ = true; return false; }
        key = row.key; return true;
    }
};

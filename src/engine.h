#pragma once

#include <algorithm>
#include <limits>
#include <cstddef>
#include <utility>
#include <vector>
struct Source {
    virtual ~Source()=default;
    virtual void Init()=0;
    virtual bool Next(int&)=0;
};
class Scan final : public Source {
    std::vector<int> values_;
    std::size_t pos_=0;
public:
    std::size_t calls=0, reads=0;
    explicit Scan(std::vector<int> values) : values_(std::move(values)) {}
    void Init() override { pos_=calls=reads=0; }
    bool Next(int& value) override {
        ++calls;
        if(pos_==values_.size()) return false;
        value=values_[pos_++]; ++reads; return true;
    }
};
class Limit final : public Source {
    Source& child_;
    std::size_t limit_, offset_, skipped_=0, emitted_=0;
    bool done_=false;
public:
    Limit(Source& child, std::size_t limit, std::size_t offset)
        : child_(child), limit_(limit), offset_(offset) {}
    void Init() override { child_.Init(); skipped_=emitted_=0; done_=false; }
    bool Next(int& value) override {
        if(done_ || emitted_==limit_) { done_=true; return false; }
        int discarded;
        while(skipped_<offset_) {
            if(!child_.Next(discarded)) { done_=true; return false; }
            ++skipped_;
        }
        int next;
        if(!child_.Next(next)) { done_=true; return false; }
        value=next; ++emitted_; return true;
    }
};

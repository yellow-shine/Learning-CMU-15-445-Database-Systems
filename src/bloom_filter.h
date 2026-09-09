#pragma once
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

class BloomFilter {
    size_t bits_, hashes_;
    uint64_t seed_;
    std::vector<uint8_t> bytes_;
    static uint64_t mix(uint64_t x) {
        x=(x^(x>>30))*UINT64_C(0xbf58476d1ce4e5b9);
        x=(x^(x>>27))*UINT64_C(0x94d049bb133111eb);
        return x^(x>>31);
    }
    size_t position(uint64_t key, size_t i) const {
        return mix(key+seed_+UINT64_C(0x9e3779b97f4a7c15)*(i+1))%bits_;
    }
public:
    BloomFilter(size_t bits, size_t hashes, uint64_t seed=37)
        : bits_(bits), hashes_(hashes), seed_(seed) {
        if (!bits || !hashes || hashes>64) throw std::invalid_argument("bits>0, hashes in [1,64]");
        bytes_.resize(bits/8+(bits%8!=0));
    }
    void add(uint64_t key) {
        for(size_t i=0;i<hashes_;++i) {
            size_t bit=position(key,i); bytes_[bit/8]|=static_cast<uint8_t>(1U<<(bit%8));
        }
    }
    bool maybe_contains(uint64_t key) const {
        for(size_t i=0;i<hashes_;++i) {
            size_t bit=position(key,i);
            if (!(bytes_[bit/8]&(1U<<(bit%8)))) return false;
        }
        return true;
    }
    void erase(uint64_t)=delete; // A shared bit must never be cleared for one key.
    double approximate_false_positive_rate(size_t distinct_insertions) const {
        return std::pow(-std::expm1(-static_cast<double>(hashes_)*distinct_insertions/bits_),
                        static_cast<double>(hashes_));
    }
};

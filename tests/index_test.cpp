#include "bloom_filter.h"
#include <iostream>
#include <limits>
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (false)
int main() {
    for(auto [bits,hashes]:std::vector<std::pair<size_t,size_t>>{{0,1},{1,0},{8,65}}) {
        bool threw=false; try { BloomFilter bad(bits,hashes); } catch(const std::invalid_argument&) {threw=true;}
        CHECK(threw);
    }
    BloomFilter filter(100003,7,37);
    CHECK(!filter.maybe_contains(0)); CHECK(filter.approximate_false_positive_rate(0)==0);
    for(uint64_t i=0;i<10000;++i) { filter.add(i); filter.add(i); }
    filter.add(std::numeric_limits<uint64_t>::max());
    CHECK(filter.maybe_contains(std::numeric_limits<uint64_t>::max()));
    for(uint64_t i=0;i<10000;++i) CHECK(filter.maybe_contains(i));
    size_t positives=0;
    for(uint64_t i=10000;i<110000;++i) positives+=filter.maybe_contains(i);
    std::cout << "observed false positives=" << positives << "/100000 (not a probability assertion)\n";
    // Deterministic collision: one bit is saturated by any insertion.
    BloomFilter saturated(1,1); saturated.add(42);
    CHECK(saturated.maybe_contains(999)); CHECK(saturated.maybe_contains(42));
    for(size_t bits:{7U,8U,9U,63U,64U,65U}) {
        BloomFilter edge(bits,64); edge.add(17); CHECK(edge.maybe_contains(17));
    }
}

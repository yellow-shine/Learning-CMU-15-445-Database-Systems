#include "bloom_filter.h"
#include <iomanip>
#include <iostream>
int main() {
    BloomFilter filter(100003,7,37);
    for(uint64_t i=0;i<10000;++i) filter.add(i);
    size_t positives=0;
    for(uint64_t i=10000;i<110000;++i) positives+=filter.maybe_contains(i);
    std::cout << "inserted42=" << filter.maybe_contains(42) << " false_positives=" << positives
              << "/100000 estimate=" << std::fixed << std::setprecision(4)
              << filter.approximate_false_positive_rate(10000) << '\n';
}

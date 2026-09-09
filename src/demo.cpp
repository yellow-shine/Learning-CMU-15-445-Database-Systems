#include "ivf_index.h"
#include <iomanip>
#include <iostream>
int main() {
    vectors::ExactIndex exact(2); std::mt19937 rng(39);
    for(int i=0;i<500;++i) exact.add({double(rng()%1000)/10,double(rng()%1000)/10});
    vectors::IVFIndex index(exact.rows(),2,12);
    vectors::Vector query{50,50}; auto target=exact.search(query,10);
    for(size_t probes:{1U,4U,12U}) {
        auto result=index.search(query,10,probes);
        std::cout << "nprobe=" << probes << " candidates=" << result.candidates
                  << " recall@10=" << std::fixed << std::setprecision(2)
                  << vectors::recall_at_k(result.hits,target) << '\n';
    }
}

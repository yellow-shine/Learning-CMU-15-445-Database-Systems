#include "ivf_index.h"
#include <iostream>
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (false)
template<class F> void rejects(F f) {bool caught=false;try{f();}catch(const std::invalid_argument&){caught=true;}CHECK(caught);}
int main() {
    using namespace vectors;
    rejects([]{IVFIndex bad({},2,1);});
    rejects([]{IVFIndex bad({{1,2}},0,1);});
    rejects([]{IVFIndex bad({{1,2}},2,0);});
    rejects([]{IVFIndex bad({{1,2}},2,2);});
    rejects([]{IVFIndex bad({{1,2}},2,1,0);});
    rejects([]{IVFIndex bad({{1}},2,1);});
    rejects([]{IVFIndex bad({{NAN,0}},2,1);});
    ExactIndex exact(2); std::mt19937 rng(39);
    for(int i=0;i<500;++i) exact.add({double(rng()%1000)/10,double(rng()%1000)/10});
    IVFIndex index(exact.rows(),2,12), repeated(exact.rows(),2,12);
    CHECK(index.centers()==repeated.centers()); CHECK(index.lists()==repeated.lists());
    std::vector<size_t> seen(500,0);
    for(size_t c=0;c<index.lists().size();++c) for(size_t id:index.lists()[c]) {
        CHECK(id<500); ++seen[id];
        size_t best=0;
        for(size_t j=1;j<index.centers().size();++j)
            if(distance(exact.rows()[id],index.centers()[j],Metric::squared_l2)<
               distance(exact.rows()[id],index.centers()[best],Metric::squared_l2)) best=j;
        CHECK(best==c);
    }
    for(auto count:seen) CHECK(count==1);
    rejects([&]{index.search({0,0},1,0);}); rejects([&]{index.search({0,0},1,13);});
    rejects([&]{index.search({0},0,1);}); rejects([&]{index.search({INFINITY,0},0,1);});
    CHECK(index.search({0,0},0,1).hits.empty());
    double recall1=0,recall4=0; size_t candidates1=0;
    for(int q=0;q<80;++q) {
        Vector query{double(rng()%1000)/10,double(rng()%1000)/10};
        auto target=exact.search(query,10);
        auto one=index.search(query,10,1),four=index.search(query,10,4),all=index.search(query,10,12);
        CHECK(all.hits==target && all.candidates==500);
        double r1=recall_at_k(one.hits,target),r4=recall_at_k(four.hits,target);
        CHECK(r1<=r4 && r4<=1); CHECK(one.candidates<=four.candidates);
        recall1+=r1;recall4+=r4;candidates1+=one.candidates;
        CHECK(index.search(query,600,12).hits==exact.search(query,600));
    }
    CHECK(candidates1<80*500);
    IVFIndex duplicates({{1,1},{1,1},{1,1},{1,1}},2,4,3);
    CHECK(duplicates.empty_updates()==9); CHECK(duplicates.lists()[0].size()==4);
    auto dup=duplicates.search({1,1},10,4); CHECK(dup.hits.size()==4);
    for(size_t i=0;i<4;++i) CHECK(dup.hits[i].id==i && dup.hits[i].distance==0);
    IVFIndex single({{2,3}},2,1); CHECK(single.search({2,3},2,1).hits[0].distance==0);
    CHECK(recall_at_k({}, {})==1);
    std::cout << "recall@10 nprobe1=" << recall1/80 << " nprobe4=" << recall4/80 << " all=1\n";
}

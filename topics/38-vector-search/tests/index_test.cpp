#include "vector_search.h"
#include <iostream>
#include <random>
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (false)
template<class F> void rejects(F f) { bool caught=false; try{f();}catch(const std::invalid_argument&){caught=true;} CHECK(caught); }
int main() {
    using namespace vectors;
    rejects([]{ExactIndex bad(0);});
    ExactIndex index(2); CHECK(index.search({0,0},3).empty());
    rejects([&]{index.search({0},0);}); rejects([&]{index.add({0});});
    rejects([&]{index.add({0,std::numeric_limits<double>::infinity()});});
    rejects([&]{index.search({0,std::numeric_limits<double>::quiet_NaN()},1);});
    index.add({1,0}); index.add({-1,0}); index.add({0,2});
    auto hits=index.search({0,0},99);
    CHECK(hits.size()==3 && hits[0].id==0 && hits[1].id==1 && hits[2].distance==4);
    CHECK(index.search({0,0},0).empty()); CHECK(index.search({0,0},1).size()==1);
    CHECK(distance({1,2},{4,6},Metric::squared_l2)==25);
    CHECK(std::abs(distance({1,0},{0,1},Metric::cosine)-1)<1e-12);
    CHECK(std::abs(distance({1,0},{-1,0},Metric::cosine)-2)<1e-12);
    CHECK(distance({1e300,0},{1e300,0},Metric::cosine)==0);
    ExactIndex cosine(2,Metric::cosine);
    rejects([&]{cosine.add({0,0});}); rejects([&]{cosine.search({0,0},0);});
    cosine.add({3,0}); cosine.add({0,4}); CHECK(cosine.search({1,0},1)[0].id==0);
    bool overflow=false;
    try{distance({1e300},{-1e300},Metric::squared_l2);}catch(const std::overflow_error&){overflow=true;}
    CHECK(overflow);
    std::mt19937 rng(38); ExactIndex random(3);
    for(int i=0;i<300;++i) random.add({double(rng()%100),double(rng()%100),double(rng()%100)});
    for(int q=0;q<60;++q) {
        Vector query{double(rng()%100),double(rng()%100),double(rng()%100)};
        std::vector<Hit> oracle;
        for(size_t id=0;id<random.rows().size();++id) {
            double sum=0;
            for(size_t j=0;j<3;++j) {double d=random.rows()[id][j]-query[j];sum+=d*d;}
            oracle.push_back({id,sum});
        }
        std::sort(oracle.begin(),oracle.end(),[](auto a,auto b){return a.distance==b.distance?a.id<b.id:a.distance<b.distance;});
        for(size_t k:{0U,1U,17U,300U,400U}) {
            auto want=oracle; want.resize(std::min(k,want.size())); CHECK(random.search(query,k)==want);
        }
    }
    std::cout << "exact top-k: metric, boundary, ties and full-sort oracle passed\n";
}

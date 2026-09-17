#pragma once
#include "vector_search.h"
#include <numeric>
#include <random>

namespace vectors {
struct SearchResult {
    std::vector<Hit> hits;
    size_t candidates=0;
};
class IVFIndex {
    size_t dimension_;
    std::vector<Vector> rows_,centers_;
    std::vector<std::vector<size_t>> lists_;
    size_t empty_updates_=0;
    size_t nearest_center(const Vector& row) const {
        size_t best=0; double best_distance=distance(row,centers_[0],Metric::squared_l2);
        for(size_t c=1;c<centers_.size();++c) {
            double d=distance(row,centers_[c],Metric::squared_l2);
            if(d<best_distance) { best=c; best_distance=d; }
        }
        return best;
    }
public:
    IVFIndex(std::vector<Vector> rows,size_t dimension,size_t nlist,
             size_t iterations=12,uint32_t seed=39):dimension_(dimension),rows_(std::move(rows)) {
        if(!dimension || !nlist || nlist>rows_.size() || !iterations)
            throw std::invalid_argument("dimension/iterations>0 and 1<=nlist<=N");
        for(const auto& row:rows_) validate(row,dimension_);
        std::vector<size_t> ids(rows_.size()); std::iota(ids.begin(),ids.end(),0);
        std::mt19937 rng(seed); std::shuffle(ids.begin(),ids.end(),rng);
        for(size_t c=0;c<nlist;++c) centers_.push_back(rows_[ids[c]]);
        for(size_t iteration=0;iteration<iterations;++iteration) {
            std::vector<Vector> means(nlist,Vector(dimension_,0));
            std::vector<size_t> count(nlist,0);
            for(const auto& row:rows_) {
                size_t c=nearest_center(row); ++count[c];
                for(size_t d=0;d<dimension_;++d) {
                    means[c][d]+=(row[d]-means[c][d])/static_cast<double>(count[c]);
                    if(!std::isfinite(means[c][d])) throw std::overflow_error("centroid overflow");
                }
            }
            for(size_t c=0;c<nlist;++c) {
                if(count[c]) centers_[c]=std::move(means[c]);
                else ++empty_updates_; // Keep previous center; do not divide by zero.
            }
        }
        lists_.resize(nlist);
        // Reassign after the LAST centroid update; old memberships would be stale.
        for(size_t id=0;id<rows_.size();++id) lists_[nearest_center(rows_[id])].push_back(id);
    }
    const auto& centers() const {return centers_;}
    const auto& lists() const {return lists_;}
    size_t empty_updates() const {return empty_updates_;}
    SearchResult search(const Vector& query,size_t k,size_t nprobe) const {
        validate(query,dimension_);
        if(!nprobe || nprobe>lists_.size()) throw std::invalid_argument("1<=nprobe<=nlist");
        SearchResult result;
        if(!k) return result;
        std::vector<Hit> coarse;
        for(size_t c=0;c<centers_.size();++c)
            coarse.push_back({c,distance(query,centers_[c],Metric::squared_l2)});
        auto probes=top_k(std::move(coarse),nprobe);
        for(auto center:probes) for(size_t id:lists_[center.id])
            result.hits.push_back({id,distance(query,rows_[id],Metric::squared_l2)});
        result.candidates=result.hits.size();
        result.hits=top_k(std::move(result.hits),k); return result;
    }
};
inline double recall_at_k(const std::vector<Hit>& approximate,const std::vector<Hit>& exact) {
    if(exact.empty()) return 1; // Empty target is vacuously recovered.
    size_t found=0;
    for(auto target:exact)
        if(std::any_of(approximate.begin(),approximate.end(),[&](auto hit){return hit.id==target.id;})) ++found;
    return static_cast<double>(found)/exact.size();
}
}

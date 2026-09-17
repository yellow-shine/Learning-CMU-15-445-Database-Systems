#pragma once
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace vectors {
using Vector=std::vector<double>;
enum class Metric { squared_l2, cosine };
struct Hit {
    size_t id;
    double distance;
    bool operator==(const Hit& other) const { return id==other.id && distance==other.distance; }
};
inline void validate(const Vector& v, size_t dimension) {
    if (v.size()!=dimension || dimension==0) throw std::invalid_argument("vector dimension");
    for(double x:v) if (!std::isfinite(x)) throw std::invalid_argument("non-finite coordinate");
}
inline double distance(const Vector& a, const Vector& b, Metric metric) {
    validate(a,b.size()); validate(b,a.size());
    if (metric==Metric::squared_l2) {
        double sum=0;
        for(size_t i=0;i<a.size();++i) { double d=a[i]-b[i]; sum+=d*d; }
        if (!std::isfinite(sum)) throw std::overflow_error("squared L2 overflow");
        return sum;
    }
    if (metric!=Metric::cosine) throw std::invalid_argument("unknown metric");
    // Scale first so huge/small finite coordinates do not overflow norms.
    double sa=0,sb=0;
    for(size_t i=0;i<a.size();++i) { sa=std::max(sa,std::abs(a[i])); sb=std::max(sb,std::abs(b[i])); }
    if (sa==0 || sb==0) throw std::invalid_argument("cosine of zero vector");
    double dot=0,aa=0,bb=0;
    for(size_t i=0;i<a.size();++i) {
        double x=a[i]/sa,y=b[i]/sb; dot+=x*y; aa+=x*x; bb+=y*y;
    }
    double cosine=dot/std::sqrt(aa)/std::sqrt(bb);
    return 1-std::clamp(cosine,-1.0,1.0);
}
inline bool nearer(const Hit& a,const Hit& b) {
    return a.distance<b.distance || (a.distance==b.distance && a.id<b.id);
}
inline std::vector<Hit> top_k(std::vector<Hit> hits,size_t k) {
    k=std::min(k,hits.size());
    if(k<hits.size()) std::nth_element(hits.begin(),hits.begin()+k,hits.end(),nearer);
    hits.resize(k); std::sort(hits.begin(),hits.end(),nearer); return hits;
}
class ExactIndex {
    size_t dimension_;
    Metric metric_;
    std::vector<Vector> rows_;
public:
    explicit ExactIndex(size_t dimension,Metric metric=Metric::squared_l2)
        :dimension_(dimension),metric_(metric) {
        if (!dimension || (metric!=Metric::squared_l2 && metric!=Metric::cosine))
            throw std::invalid_argument("dimension or metric");
    }
    size_t add(Vector value) {
        validate(value,dimension_);
        if(metric_==Metric::cosine) (void)distance(value,value,metric_);
        rows_.push_back(std::move(value)); return rows_.size()-1;
    }
    const std::vector<Vector>& rows() const { return rows_; }
    std::vector<Hit> search(const Vector& query,size_t k) const {
        validate(query,dimension_);
        if(metric_==Metric::cosine) (void)distance(query,query,metric_);
        std::vector<Hit> hits;
        if (!k) return hits;
        hits.reserve(rows_.size());
        for(size_t i=0;i<rows_.size();++i) hits.push_back({i,distance(query,rows_[i],metric_)});
        return top_k(std::move(hits),k);
    }
};
}

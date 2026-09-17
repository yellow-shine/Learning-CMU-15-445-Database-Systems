#pragma once
#include <algorithm>
#include <cstdint>
#include <functional>
#include <utility>
#include <stdexcept>
#include <vector>
namespace tutorial {
inline std::size_t hash_route(std::int64_t key, std::size_t partitions) {
  if (partitions == 0) throw std::invalid_argument("zero partitions");
  return static_cast<std::uint64_t>(key) % partitions;
}
class RangeRouter {
  std::vector<std::int64_t> cuts_;
 public:
  explicit RangeRouter(std::vector<std::int64_t> cuts): cuts_(std::move(cuts)) {
    if (std::adjacent_find(cuts_.begin(), cuts_.end(), std::greater_equal<std::int64_t>()) != cuts_.end())
      throw std::invalid_argument("cuts must strictly increase");
  }
  std::size_t route(std::int64_t key) const {
    return static_cast<std::size_t>(std::upper_bound(cuts_.begin(), cuts_.end(), key)-cuts_.begin());
  }
  std::size_t partitions() const { return cuts_.size()+1; }
};
struct Statistics { std::vector<std::size_t> counts; double skew; };
template<class Route>
Statistics statistics(const std::vector<std::int64_t>& keys, std::size_t partitions, Route route) {
  if (!partitions) throw std::invalid_argument("zero partitions");
  Statistics s{std::vector<std::size_t>(partitions),0};
  for (auto key: keys) {
    auto p=route(key);
    if(p>=partitions) throw std::out_of_range("route outside partitions");
    ++s.counts[p];
  }
  if (!keys.empty()) s.skew=static_cast<double>(*std::max_element(s.counts.begin(),s.counts.end()))*static_cast<double>(partitions)/static_cast<double>(keys.size());
  return s;
}
}

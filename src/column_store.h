#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>
namespace storage {
struct Row {
  std::uint32_t id, score, age;
  bool operator==(const Row& r) const { return id==r.id && score==r.score && age==r.age; }
};
class ColumnStore {
  std::array<std::vector<std::uint32_t>,3> columns_;
 public:
  explicit ColumnStore(const std::vector<Row>& rows) {
    for(auto& col:columns_) col.reserve(rows.size());
    for(auto r:rows) { columns_[0].push_back(r.id); columns_[1].push_back(r.score); columns_[2].push_back(r.age); }
  }
  std::size_t size() const { return columns_[0].size(); }
  const std::vector<std::uint32_t>& column(std::size_t c) const { return columns_.at(c); }
  Row get(std::size_t i,std::size_t& scalar_reads) const {
    if(i>=size()) throw std::out_of_range("row position");
    scalar_reads+=3; return {columns_[0][i],columns_[1][i],columns_[2][i]};
  }
  std::vector<std::size_t> select(std::uint32_t threshold,std::size_t& scalar_reads) const {
    std::vector<std::size_t> positions;
    for(std::size_t i=0;i<size();++i) { ++scalar_reads; if(columns_[1][i]>=threshold) positions.push_back(i); }
    return positions;
  }
  std::vector<std::uint32_t> project_ids(const std::vector<std::size_t>& positions,std::size_t& scalar_reads) const {
    // Validate the entire position list before recording any access.
    for(auto i:positions) if(i>=size()) throw std::out_of_range("row position");
    std::vector<std::uint32_t> ids; ids.reserve(positions.size());
    for(auto i:positions) ids.push_back(columns_[0][i]);
    // The counter may alias a position; update only after all positions are consumed.
    scalar_reads+=positions.size();
    return ids;
  }
};
}

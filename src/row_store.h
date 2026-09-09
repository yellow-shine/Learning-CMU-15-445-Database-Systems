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
// Canonical little-endian fields, never memcpy a padded C++ Row object.
class RowStore {
  std::vector<unsigned char> bytes_;
  std::uint32_t field(std::size_t offset) const {
    std::uint32_t n=0;
    for(unsigned b=0;b<4;++b) n|=std::uint32_t(bytes_.at(offset+b))<<(8*b);
    return n;
  }
 public:
  explicit RowStore(const std::vector<Row>& rows) {
    if(rows.size()>bytes_.max_size()/12) throw std::length_error("row store too large");
    bytes_.reserve(rows.size()*12);
    for(auto r:rows) for(auto n:std::array<std::uint32_t,3>{r.id,r.score,r.age})
      for(unsigned b=0;b<4;++b) bytes_.push_back(static_cast<unsigned char>(n>>(8*b)));
  }
  std::size_t size() const { return bytes_.size()/12; }
  const std::vector<unsigned char>& bytes() const { return bytes_; }
  Row get(std::size_t i,std::size_t& scalar_reads) const {
    if(i>=size()) throw std::out_of_range("row id");
    scalar_reads+=3; return {field(i*12),field(i*12+4),field(i*12+8)};
  }
  std::vector<std::size_t> select(std::uint32_t threshold,std::size_t& scalar_reads) const {
    std::vector<std::size_t> positions;
    // This row-at-a-time operator materializes whole rows, not a projection-optimized scan.
    for(std::size_t i=0;i<size();++i) if(get(i,scalar_reads).score>=threshold) positions.push_back(i);
    return positions;
  }
};
}

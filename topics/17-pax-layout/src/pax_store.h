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
class PaxStore {
 public:
  static constexpr std::size_t page_size=128, capacity=10, header_size=8;
  using Page=std::array<unsigned char,page_size>;
  static std::size_t offset(std::size_t column,std::size_t local_row) {
    if(column>=3 || local_row>=capacity) throw std::out_of_range("mini-page coordinate");
    return header_size+column*capacity*4+local_row*4;
  }
 private:
  std::vector<Page> pages_;
  static std::uint32_t field(const Page& p,std::size_t col,std::size_t row) {
    const auto at=offset(col,row); std::uint32_t n=0;
    for(unsigned b=0;b<4;++b) n|=std::uint32_t(p[at+b])<<(8*b);
    return n;
  }
 public:
  explicit PaxStore(const std::vector<Row>& rows) {
    for(std::size_t i=0;i<rows.size();++i) {
      if(i%capacity==0) { Page p{}; p[0]='P';p[1]='A';p[2]='X';p[3]=1;pages_.push_back(p); }
      auto& p=pages_.back(); auto r=rows[i]; std::array<std::uint32_t,3> values{r.id,r.score,r.age};
      for(std::size_t c=0;c<3;++c) for(unsigned b=0;b<4;++b)
        p[offset(c,i%capacity)+b]=static_cast<unsigned char>(values[c]>>(8*b));
      ++p[4];
    }
  }
  static PaxStore from_pages(const std::vector<Page>& pages) {
    for(std::size_t i=0;i<pages.size();++i) {
      const auto& p=pages[i];
      if(p[0]!='P'||p[1]!='A'||p[2]!='X'||p[3]!=1||p[4]==0||p[4]>capacity||p[5]||p[6]||p[7]
         ||(i+1<pages.size() && p[4]!=capacity)) throw std::runtime_error("invalid PAX header");
      for(std::size_t c=0;c<3;++c) for(std::size_t r=p[4];r<capacity;++r)
        if(field(p,c,r)!=0) throw std::runtime_error("nonzero unused mini-page entry");
    }
    PaxStore result(std::vector<Row>{}); result.pages_=pages; return result;
  }
  const std::vector<Page>& pages() const { return pages_; }
  std::size_t size() const { return pages_.empty()?0:(pages_.size()-1)*capacity+pages_.back()[4]; }
  Row get(std::size_t i,std::size_t& scalar_reads) const {
    if(i>=size()) throw std::out_of_range("row position");
    const auto& p=pages_[i/capacity]; const auto r=i%capacity;
    scalar_reads+=3; return {field(p,0,r),field(p,1,r),field(p,2,r)};
  }
  std::vector<std::size_t> select(std::uint32_t threshold,std::size_t& scalar_reads) const {
    std::vector<std::size_t> positions;
    for(std::size_t page=0;page<pages_.size();++page)
      for(std::size_t r=0;r<pages_[page][4];++r) {
        ++scalar_reads;
        if(field(pages_[page],1,r)>=threshold) positions.push_back(page*capacity+r);
      }
    return positions;
  }
};
}

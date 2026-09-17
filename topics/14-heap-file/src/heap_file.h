#pragma once
#include "page_file.h"
#include "slotted_page.h"
#include <vector>
namespace storage {
struct RID { std::uint64_t page; std::size_t slot; };
class HeapFile {
  PageFile file_;
 public:
  explicit HeapFile(const std::filesystem::path& path, bool create=false):file_(path,create) {
    // Validate every existing page before accepting the heap.
    for(std::uint64_t i=0;i<file_.size();++i) { SlottedPage checked(file_.read(i)); }
  }
  std::uint64_t pages() const { return file_.size(); }
  RID insert(const std::string& value) {
    SlottedPage fresh;
    auto new_slot=fresh.insert(value);
    if(!new_slot) throw std::length_error("record exceeds one page");
    // ponytail: first-fit scans all pages; use a free-space map for large heaps.
    for(std::uint64_t i=0;i<pages();++i) {
      SlottedPage p(file_.read(i));
      if(auto slot=p.insert(value)) { file_.write(i,p.bytes()); return {i,*slot}; }
    }
    return {file_.append(fresh.bytes()),*new_slot};
  }
  std::optional<std::string> get(RID rid) { return SlottedPage(file_.read(rid.page)).get(rid.slot); }
  void erase(RID rid) {
    SlottedPage p(file_.read(rid.page)); p.erase(rid.slot); file_.write(rid.page,p.bytes());
  }
  bool update(RID rid,const std::string& value) {
    SlottedPage p(file_.read(rid.page));
    if(!p.update(rid.slot,value)) return false;
    file_.write(rid.page,p.bytes()); return true;
  }
  std::vector<std::pair<RID,std::string>> scan() {
    std::vector<std::pair<RID,std::string>> rows;
    for(std::uint64_t i=0;i<pages();++i) {
      SlottedPage p(file_.read(i));
      for(std::size_t j=0;j<p.slots();++j) if(auto value=p.get(j)) rows.push_back({{i,j},*value});
    }
    return rows;
  }
};
}

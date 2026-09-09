#pragma once
#include "disk.h"
#include <optional>
#include <unordered_map>
#include <vector>

struct Frame {
  Page data{};
  std::optional<std::size_t> page_id;
  std::size_t pins = 0;
  bool dirty = false;
};
// Single-threaded pool. A returned reference is valid only while pinned.
class BufferPool {
 public:
  BufferPool(Disk& disk, std::size_t capacity) : disk_(disk), frames_(capacity) {
    if (!capacity) throw std::invalid_argument("zero frame capacity");
  }
  BufferPool(const BufferPool&) = delete;
  BufferPool& operator=(const BufferPool&) = delete;
  Frame& fetch(std::size_t id) {
    auto hit = table_.find(id);
    if (hit != table_.end()) {
      auto& frame = frames_[hit->second];
      if (frame.pins == std::numeric_limits<std::size_t>::max()) throw std::overflow_error("pins");
      ++frame.pins;
      return frame;
    }
    // ponytail: linear first-unpinned victim, use a replacer for larger pools.
    std::size_t victim = 0;
    while (victim < frames_.size() && frames_[victim].pins) ++victim;
    if (victim == frames_.size()) throw std::runtime_error("all frames pinned");
    Page incoming = disk_.read(id);  // Failed read leaves resident mappings intact.
    auto& frame = frames_[victim];
    flush_frame(frame);             // Failed write retains dirty victim for retry.
    table_.emplace(id, victim);     // Allocate before changing resident state.
    if (frame.page_id) table_.erase(*frame.page_id);
    frame.data = incoming;
    frame.page_id = id;
    frame.pins = 1;
    frame.dirty = false;
    return frame;
  }
  void unpin(std::size_t id, bool dirty = false) {
    auto& frame = frames_.at(table_.at(id));
    if (!frame.pins) throw std::logic_error("double unpin");
    frame.dirty = frame.dirty || dirty;
    --frame.pins;
  }
  void flush(std::size_t id) { flush_frame(frames_.at(table_.at(id))); }
  void flush_all() { for (auto& frame : frames_) flush_frame(frame); }
  std::size_t resident() const { return table_.size(); }
  std::size_t pins(std::size_t id) const { return frames_.at(table_.at(id)).pins; }
 private:
  void flush_frame(Frame& frame) {
    if (frame.dirty) { disk_.write(*frame.page_id, frame.data); frame.dirty = false; }
  }
  Disk& disk_;
  std::vector<Frame> frames_;
  std::unordered_map<std::size_t, std::size_t> table_;
};

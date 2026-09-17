#pragma once
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct Lifetime {
  int constructed = 0;
  int destroyed = 0;
};

// Counters are shared too, so their lifetime cannot end before the observed object.
struct Page {
  int id;
  std::shared_ptr<Lifetime> lifetime;
  Page(int page_id, std::shared_ptr<Lifetime> counts)
      : id(page_id), lifetime(std::move(counts)) {
    if (!lifetime) throw std::invalid_argument("null lifetime counter");
    ++lifetime->constructed;
  }
  Page(const Page &) = delete;
  Page &operator=(const Page &) = delete;
  ~Page() { ++lifetime->destroyed; }
};

// An owning tree points down; the reverse navigation edge is non-owning.
struct PlanNode {
  std::string name;
  std::vector<std::shared_ptr<PlanNode>> children;
  std::weak_ptr<PlanNode> parent;
  std::shared_ptr<Lifetime> lifetime;
  PlanNode(std::string label, std::shared_ptr<Lifetime> counts)
      : name(std::move(label)), lifetime(std::move(counts)) {
    if (!lifetime) throw std::invalid_argument("null lifetime counter");
    ++lifetime->constructed;
  }
  PlanNode(const PlanNode &) = delete;
  PlanNode &operator=(const PlanNode &) = delete;
  ~PlanNode() { ++lifetime->destroyed; }
};

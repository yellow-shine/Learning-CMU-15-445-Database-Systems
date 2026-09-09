#include "ownership.h"
#include "check.h"
#include <type_traits>

// Deliberately strong back edge: break it after observing the retained cycle.
struct CycleNode {
  std::shared_ptr<CycleNode> next;
  std::shared_ptr<Lifetime> counts;
  explicit CycleNode(std::shared_ptr<Lifetime> c) : counts(std::move(c)) { ++counts->constructed; }
  ~CycleNode() { ++counts->destroyed; }
};
int main() {
  static_assert(!std::is_copy_constructible_v<std::unique_ptr<Page>>);
  auto counts = std::make_shared<Lifetime>();
  std::weak_ptr<Page> observer;
  { auto one = std::make_unique<Page>(7, counts); auto *address = one.get();
    auto two = std::move(one); CHECK(!one && two.get() == address);
    std::shared_ptr<Page> shared = std::move(two); CHECK(!two && shared.get() == address);
    observer = shared;
    { auto reader = shared; CHECK(reader.use_count() == 2);
      auto pinned = observer.lock(); CHECK(pinned && pinned->id == 7 && shared.use_count() == 3);
    }
    CHECK(shared.use_count() == 1 && counts->destroyed == 0);
  }
  CHECK(counts->constructed == 1 && counts->destroyed == 1);
  CHECK(observer.expired() && !observer.lock());
  auto nodes = std::make_shared<Lifetime>();
  auto root = std::make_shared<PlanNode>("filter", nodes);
  auto child = std::make_shared<PlanNode>("scan", nodes);
  root->children.push_back(child); child->parent = root;
  CHECK(child->parent.lock() == root);
  root.reset(); CHECK(nodes->destroyed == 1 && child->parent.expired());
  child.reset(); CHECK(nodes->destroyed == 2);
  auto cycle_counts = std::make_shared<Lifetime>();
  std::weak_ptr<CycleNode> probe;
  { auto a = std::make_shared<CycleNode>(cycle_counts);
    auto b = std::make_shared<CycleNode>(cycle_counts);
    a->next = b; b->next = a; probe = a;
  }
  CHECK(!probe.expired() && cycle_counts->destroyed == 0);
  { auto a = probe.lock(); auto b = a->next; b->next.reset(); a->next.reset(); }
  CHECK(probe.expired() && cycle_counts->destroyed == 2);
  bool failed = false;
  try { Page invalid(0, nullptr); } catch (const std::invalid_argument &) { failed = true; }
  CHECK(failed);
}

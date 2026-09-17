#include "ownership.h"
#include <iostream>
int main() {
  auto pages = std::make_shared<Lifetime>();
  auto owner = std::make_unique<Page>(7, pages);
  Page *borrowed = owner.get();
  std::cout << "borrowed page: " << borrowed->id << '\n';
  auto moved = std::move(owner);
  std::shared_ptr<Page> shared = std::move(moved);
  std::weak_ptr<Page> observer = shared;
  { auto reader = shared; std::cout << "strong owners: " << shared.use_count() << '\n'; }
  shared.reset();
  std::cout << "expired: " << observer.expired() << ", destroyed: " << pages->destroyed << '\n';
  auto nodes = std::make_shared<Lifetime>();
  auto root = std::make_shared<PlanNode>("filter", nodes);
  auto scan = std::make_shared<PlanNode>("scan", nodes);
  root->children.push_back(scan); scan->parent = root;
  root.reset();
  std::cout << "scan parent expired: " << scan->parent.expired() << '\n';
  scan.reset();
  std::cout << "plan nodes destroyed: " << nodes->destroyed << '\n';
}

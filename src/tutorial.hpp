#pragma once
#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace tutorial {

enum class Kind { Read, Write };
struct Operation { int transaction; std::string key; Kind kind; };
using Schedule = std::vector<Operation>;
using Graph = std::map<int, std::set<int>>;
Graph precedence(const Schedule& schedule) {
  Graph graph;
  for (const auto& op : schedule) {
    if (op.transaction < 0 || op.key.empty()) throw std::invalid_argument("invalid operation");
    graph[op.transaction];
  }
  for (std::size_t i=0; i<schedule.size(); ++i)
    for (std::size_t j=i+1; j<schedule.size(); ++j) {
      const auto& a=schedule[i]; const auto& b=schedule[j];
      if (a.transaction != b.transaction && a.key == b.key &&
          (a.kind == Kind::Write || b.kind == Kind::Write))
        graph[a.transaction].insert(b.transaction);
    }
  return graph;
}
// Kahn elimination returns an actual serial order, not merely a cycle flag.
std::optional<std::vector<int>> serial_order(const Graph& graph) {
  std::map<int,std::size_t> indegree;
  for (const auto& [v, edges] : graph) {
    indegree.try_emplace(v,0);
    for (int next : edges) ++indegree[next];
  }
  std::set<int> ready;
  for (const auto& [v, degree] : indegree) if (degree==0) ready.insert(v);
  std::vector<int> order;
  while (!ready.empty()) {
    int v=*ready.begin(); ready.erase(ready.begin()); order.push_back(v);
    auto it=graph.find(v);
    if (it!=graph.end()) for (int next : it->second) if (--indegree[next]==0) ready.insert(next);
  }
  if (order.size()!=indegree.size()) return std::nullopt;
  return order;
}

}

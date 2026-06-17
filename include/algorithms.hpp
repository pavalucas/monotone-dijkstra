#pragma once

#include <functional>
#include <string>
#include <vector>

#include "dijkstra.hpp"
#include "graph.hpp"

namespace sssp {

// One entry per SSSP variant. To add a variant, implement
// `dijkstra_with_queue<YourQueue>` in dijkstra.hpp and append a line to
// `algorithm_registry()` below — nothing else needs to change.
struct AlgorithmEntry {
  std::string name;
  std::function<ShortestPathResult(const Graph&, Vertex)> run;
};

inline const std::vector<AlgorithmEntry>& algorithm_registry() {
  static const std::vector<AlgorithmEntry> registry = {
      {"binary_heap", dijkstra_binary_heap},
      {"one_level_radix_heap", dijkstra_one_level_radix_heap},
  };
  return registry;
}

inline const AlgorithmEntry* find_algorithm(const std::string& name) {
  for (const AlgorithmEntry& entry : algorithm_registry()) {
    if (entry.name == name) {
      return &entry;
    }
  }
  return nullptr;
}

}  // namespace sssp

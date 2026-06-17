#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "binary_heap.hpp"
#include "graph.hpp"
#include "radix_heap.hpp"

namespace sssp {

struct ShortestPathStats {
  std::uint64_t pushes = 0;
  std::uint64_t pops = 0;
  std::uint64_t stale_pops = 0;
  std::uint64_t relax_attempts = 0;
  std::uint64_t relax_successes = 0;
};

struct ShortestPathResult {
  std::vector<Weight> dist;
  ShortestPathStats stats;
};

inline constexpr Weight INF = std::numeric_limits<Weight>::max() / 4;

template <typename Queue>
ShortestPathResult dijkstra_with_queue(const Graph& graph, Vertex source) {
  const int n = graph.vertex_count();
  if (source < 0 || source >= n) {
    throw std::out_of_range("source id out of range");
  }

  ShortestPathResult result;
  result.dist.assign(n, INF);
  result.dist[source] = 0;

  Queue queue;
  queue.push(0, source);
  ++result.stats.pushes;

  while (!queue.empty()) {
    const auto [du, u] = queue.pop_min();
    ++result.stats.pops;

    if (du != result.dist[u]) {
      ++result.stats.stale_pops;
      continue;
    }

    for (const Edge& edge : graph.outgoing(u)) {
      ++result.stats.relax_attempts;
      if (du > INF - edge.weight) {
        continue;
      }
      const Weight candidate = du + edge.weight;
      if (candidate < result.dist[edge.to]) {
        result.dist[edge.to] = candidate;
        queue.push(candidate, edge.to);
        ++result.stats.pushes;
        ++result.stats.relax_successes;
      }
    }
  }

  return result;
}

inline ShortestPathResult dijkstra_binary_heap(const Graph& graph, Vertex source) {
  return dijkstra_with_queue<BinaryHeapQueue>(graph, source);
}

inline ShortestPathResult dijkstra_one_level_radix_heap(const Graph& graph, Vertex source) {
  return dijkstra_with_queue<RadixHeapQueue>(graph, source);
}

inline std::uint64_t distance_checksum(const std::vector<Weight>& dist) {
  constexpr std::uint64_t mod = 1'000'000'007ULL;
  std::uint64_t checksum = 0;
  for (Weight value : dist) {
    checksum = (checksum * 1315423911ULL + (value == INF ? 17ULL : value % mod)) % mod;
  }
  return checksum;
}

}  // namespace sssp

#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace sssp {

using Vertex = int;
using Weight = std::uint64_t;

struct Edge {
  Vertex to;
  Weight weight;
};

class Graph {
 public:
  explicit Graph(int vertex_count) : adjacency_(vertex_count) {
    if (vertex_count < 0) {
      throw std::invalid_argument("vertex_count must be non-negative");
    }
  }

  int vertex_count() const {
    return static_cast<int>(adjacency_.size());
  }

  std::uint64_t edge_count() const {
    std::uint64_t total = 0;
    for (const auto& edges : adjacency_) {
      total += edges.size();
    }
    return total;
  }

  void add_edge(Vertex from, Vertex to, Weight weight) {
    check_vertex(from);
    check_vertex(to);
    adjacency_[from].push_back({to, weight});
  }

  const std::vector<Edge>& outgoing(Vertex vertex) const {
    check_vertex(vertex);
    return adjacency_[vertex];
  }

 private:
  void check_vertex(Vertex vertex) const {
    if (vertex < 0 || vertex >= vertex_count()) {
      throw std::out_of_range("vertex id out of range");
    }
  }

  std::vector<std::vector<Edge>> adjacency_;
};

}  // namespace sssp

#pragma once

#include <cstdint>
#include <random>
#include <stdexcept>

#include "graph.hpp"

namespace sssp {

struct GridGraphOptions {
  int rows;
  int cols;
  Weight max_weight;
  std::uint64_t seed = 1;
  bool bidirectional = true;
};

inline Vertex grid_vertex_id(int row, int col, int cols) {
  return row * cols + col;
}

inline Graph make_grid_graph(const GridGraphOptions& options) {
  if (options.rows <= 0) {
    throw std::invalid_argument("grid rows must be positive");
  }
  if (options.cols <= 0) {
    throw std::invalid_argument("grid cols must be positive");
  }
  if (options.max_weight == 0) {
    throw std::invalid_argument("grid max_weight must be positive");
  }

  Graph graph(options.rows * options.cols);
  std::mt19937_64 rng(options.seed);
  std::uniform_int_distribution<Weight> weight_dist(1, options.max_weight);

  auto add_pair = [&](Vertex from, Vertex to) {
    const Weight weight = weight_dist(rng);
    graph.add_edge(from, to, weight);
    if (options.bidirectional) {
      graph.add_edge(to, from, weight);
    }
  };

  for (int row = 0; row < options.rows; ++row) {
    for (int col = 0; col < options.cols; ++col) {
      const Vertex current = grid_vertex_id(row, col, options.cols);
      if (col + 1 < options.cols) {
        add_pair(current, grid_vertex_id(row, col + 1, options.cols));
      }
      if (row + 1 < options.rows) {
        add_pair(current, grid_vertex_id(row + 1, col, options.cols));
      }
    }
  }

  return graph;
}

struct SparseGraphOptions {
  int vertices;
  std::uint64_t edges;
  Weight max_weight;
  std::uint64_t seed = 1;
};

// Random sparse directed graph. A random spanning tree is laid down first so
// every vertex is reachable from vertex 0 (each vertex i > 0 gets an edge from a
// uniformly chosen earlier vertex); any remaining edges are random ordered pairs.
// Parallel edges are allowed, self-loops are not. If `edges` is below the
// vertices-1 needed for the tree it is raised to that minimum.
inline Graph make_sparse_graph(const SparseGraphOptions& options) {
  if (options.vertices <= 0) {
    throw std::invalid_argument("sparse vertices must be positive");
  }
  if (options.max_weight == 0) {
    throw std::invalid_argument("sparse max_weight must be positive");
  }

  Graph graph(options.vertices);
  std::mt19937_64 rng(options.seed);
  std::uniform_int_distribution<Weight> weight_dist(1, options.max_weight);

  auto add_edge = [&](Vertex from, Vertex to) {
    graph.add_edge(from, to, weight_dist(rng));
  };

  const std::uint64_t tree_edges =
      options.vertices > 1 ? static_cast<std::uint64_t>(options.vertices) - 1 : 0;
  for (int i = 1; i < options.vertices; ++i) {
    std::uniform_int_distribution<int> parent_dist(0, i - 1);
    add_edge(parent_dist(rng), i);
  }

  if (options.vertices > 1) {
    std::uniform_int_distribution<int> vertex_dist(0, options.vertices - 1);
    for (std::uint64_t added = tree_edges; added < options.edges; ++added) {
      const Vertex from = vertex_dist(rng);
      Vertex to = vertex_dist(rng);
      while (to == from) {
        to = vertex_dist(rng);
      }
      add_edge(from, to);
    }
  }

  return graph;
}

}  // namespace sssp

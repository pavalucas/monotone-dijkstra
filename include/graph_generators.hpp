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

}  // namespace sssp

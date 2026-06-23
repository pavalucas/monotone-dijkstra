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

struct DenseGraphOptions {
  int vertices;
  int density_percent;  // probability (%) that each ordered pair (u != v) is an edge
  Weight max_weight;
  std::uint64_t seed = 1;
};

// Dense Erdős–Rényi directed graph: each ordered pair (u, v), u != v, is an edge
// with probability density_percent/100, giving an expected m ≈ p·n(n-1). A
// random spanning tree is added first so every vertex is reachable from 0.
inline Graph make_dense_graph(const DenseGraphOptions& options) {
  if (options.vertices <= 0) {
    throw std::invalid_argument("dense vertices must be positive");
  }
  if (options.density_percent <= 0 || options.density_percent > 100) {
    throw std::invalid_argument("dense density_percent must be in (0, 100]");
  }
  if (options.max_weight == 0) {
    throw std::invalid_argument("dense max_weight must be positive");
  }

  Graph graph(options.vertices);
  std::mt19937_64 rng(options.seed);
  std::uniform_int_distribution<Weight> weight_dist(1, options.max_weight);
  std::bernoulli_distribution edge_dist(options.density_percent / 100.0);

  auto add_edge = [&](Vertex from, Vertex to) {
    graph.add_edge(from, to, weight_dist(rng));
  };

  for (int i = 1; i < options.vertices; ++i) {
    std::uniform_int_distribution<int> parent_dist(0, i - 1);
    add_edge(parent_dist(rng), i);
  }

  for (int u = 0; u < options.vertices; ++u) {
    for (int v = 0; v < options.vertices; ++v) {
      if (u != v && edge_dist(rng)) {
        add_edge(u, v);
      }
    }
  }

  return graph;
}

struct LayeredGraphOptions {
  int layers;  // number of width-W layers after the single super-source
  int width;
  int degree;  // extra forward edges per node into the next layer
  Weight max_weight;
  std::uint64_t seed = 1;
};

// Layered DAG: vertex 0 is a super-source feeding layer 1; layers 1..L each have
// `width` nodes with edges only to the next layer. Every node gets one guaranteed
// parent in the previous layer (so all are reachable from 0) plus `degree` extra
// random forward edges. Vertex count is 1 + layers*width.
inline Graph make_layered_graph(const LayeredGraphOptions& options) {
  if (options.layers <= 0) {
    throw std::invalid_argument("layered layers must be positive");
  }
  if (options.width <= 0) {
    throw std::invalid_argument("layered width must be positive");
  }
  if (options.degree < 0) {
    throw std::invalid_argument("layered degree must be non-negative");
  }
  if (options.max_weight == 0) {
    throw std::invalid_argument("layered max_weight must be positive");
  }

  Graph graph(1 + options.layers * options.width);
  std::mt19937_64 rng(options.seed);
  std::uniform_int_distribution<Weight> weight_dist(1, options.max_weight);
  std::uniform_int_distribution<int> idx_dist(0, options.width - 1);

  // Vertex id of node `i` in layer `layer` (layer >= 1); layer 0 is vertex 0.
  auto node_id = [&](int layer, int i) {
    return 1 + (layer - 1) * options.width + i;
  };

  auto add_edge = [&](Vertex from, Vertex to) {
    graph.add_edge(from, to, weight_dist(rng));
  };

  for (int i = 0; i < options.width; ++i) {
    add_edge(0, node_id(1, i));
  }

  for (int layer = 1; layer < options.layers; ++layer) {
    for (int i = 0; i < options.width; ++i) {
      add_edge(node_id(layer, idx_dist(rng)), node_id(layer + 1, i));
    }
    for (int i = 0; i < options.width; ++i) {
      const Vertex u = node_id(layer, i);
      for (int e = 0; e < options.degree; ++e) {
        add_edge(u, node_id(layer + 1, idx_dist(rng)));
      }
    }
  }

  return graph;
}

}  // namespace sssp

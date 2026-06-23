#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "dijkstra.hpp"
#include "graph_generators.hpp"

namespace {

using sssp::Graph;
using sssp::INF;
using sssp::ShortestPathResult;
using sssp::Weight;

void require(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

void require_dist(const ShortestPathResult& result, const std::vector<Weight>& expected) {
  require(result.dist == expected, "distance vector did not match expected values");
}

void require_same_results(const Graph& graph, int source) {
  const auto binary = sssp::dijkstra_binary_heap(graph, source);
  const auto one_level = sssp::dijkstra_one_level_radix_heap(graph, source);
  const auto two_level = sssp::dijkstra_two_level_radix_heap(graph, source);
  require(binary.dist == one_level.dist,
          "binary heap and one-level radix heap returned different distances");
  require(binary.dist == two_level.dist,
          "binary heap and two-level radix heap returned different distances");
  require(sssp::distance_checksum(binary.dist) == sssp::distance_checksum(one_level.dist),
          "binary heap and one-level radix heap checksums differ");
  require(sssp::distance_checksum(binary.dist) == sssp::distance_checksum(two_level.dist),
          "binary heap and two-level radix heap checksums differ");
}

void test_small_weighted_graph() {
  Graph graph(4);
  graph.add_edge(0, 1, 2);
  graph.add_edge(0, 2, 5);
  graph.add_edge(1, 2, 1);
  graph.add_edge(1, 3, 4);
  graph.add_edge(2, 3, 1);

  const std::vector<Weight> expected = {0, 2, 3, 4};
  require_dist(sssp::dijkstra_binary_heap(graph, 0), expected);
  require_dist(sssp::dijkstra_one_level_radix_heap(graph, 0), expected);
  require_dist(sssp::dijkstra_two_level_radix_heap(graph, 0), expected);
  require_same_results(graph, 0);
}

void test_unreachable_vertices() {
  Graph graph(5);
  graph.add_edge(0, 1, 3);
  graph.add_edge(1, 2, 4);
  graph.add_edge(3, 4, 1);

  const std::vector<Weight> expected = {0, 3, 7, INF, INF};
  require_dist(sssp::dijkstra_binary_heap(graph, 0), expected);
  require_dist(sssp::dijkstra_one_level_radix_heap(graph, 0), expected);
  require_dist(sssp::dijkstra_two_level_radix_heap(graph, 0), expected);
  require_same_results(graph, 0);
}

void test_zero_weight_edges() {
  Graph graph(4);
  graph.add_edge(0, 1, 0);
  graph.add_edge(1, 2, 0);
  graph.add_edge(0, 2, 5);
  graph.add_edge(2, 3, 2);

  const std::vector<Weight> expected = {0, 0, 0, 2};
  require_dist(sssp::dijkstra_binary_heap(graph, 0), expected);
  require_dist(sssp::dijkstra_one_level_radix_heap(graph, 0), expected);
  require_dist(sssp::dijkstra_two_level_radix_heap(graph, 0), expected);
  require_same_results(graph, 0);
}

void test_cycle_graph() {
  Graph graph(5);
  graph.add_edge(0, 1, 10);
  graph.add_edge(0, 2, 1);
  graph.add_edge(2, 1, 2);
  graph.add_edge(1, 3, 1);
  graph.add_edge(3, 2, 1);
  graph.add_edge(3, 4, 7);

  const std::vector<Weight> expected = {0, 3, 1, 4, 11};
  require_dist(sssp::dijkstra_binary_heap(graph, 0), expected);
  require_dist(sssp::dijkstra_one_level_radix_heap(graph, 0), expected);
  require_dist(sssp::dijkstra_two_level_radix_heap(graph, 0), expected);
  require_same_results(graph, 0);
}

void test_unit_weight_grid_graph() {
  const Graph graph = sssp::make_grid_graph({
      .rows = 3,
      .cols = 3,
      .max_weight = 1,
      .seed = 7,
      .bidirectional = true,
  });

  require(graph.vertex_count() == 9, "3x3 grid should have 9 vertices");
  require(graph.edge_count() == 24, "3x3 bidirectional grid should have 24 directed edges");

  const std::vector<Weight> expected = {
      0, 1, 2,
      1, 2, 3,
      2, 3, 4,
  };
  require_dist(sssp::dijkstra_binary_heap(graph, 0), expected);
  require_dist(sssp::dijkstra_one_level_radix_heap(graph, 0), expected);
  require_dist(sssp::dijkstra_two_level_radix_heap(graph, 0), expected);
  require_same_results(graph, 0);
}

void test_seeded_weighted_grid_consistency() {
  const Graph graph = sssp::make_grid_graph({
      .rows = 12,
      .cols = 10,
      .max_weight = 1000,
      .seed = 12345,
      .bidirectional = true,
  });

  require(graph.vertex_count() == 120, "12x10 grid should have 120 vertices");
  require(graph.edge_count() == 436, "12x10 bidirectional grid should have 436 directed edges");
  require_same_results(graph, 0);
  require_same_results(graph, 37);
  require_same_results(graph, 119);
}

void test_large_weight_range_consistency() {
  // Large C forces distances across many base-K digits, exercising the
  // two-level heap's redistribution across top-level buckets.
  const Graph graph = sssp::make_grid_graph({
      .rows = 40,
      .cols = 40,
      .max_weight = 1'000'000,
      .seed = 98765,
      .bidirectional = true,
  });

  require(graph.vertex_count() == 1600, "40x40 grid should have 1600 vertices");
  require_same_results(graph, 0);
  require_same_results(graph, 800);
  require_same_results(graph, 1599);
}

void test_sparse_graph_connectivity_and_consistency() {
  const Graph graph = sssp::make_sparse_graph({
      .vertices = 2000,
      .edges = 6000,
      .max_weight = 1'000'000,
      .seed = 4242,
  });

  require(graph.vertex_count() == 2000, "sparse graph should have 2000 vertices");
  require(graph.edge_count() == 6000, "sparse graph should have 6000 edges");

  // The random spanning tree guarantees every vertex is reachable from 0.
  const auto result = sssp::dijkstra_binary_heap(graph, 0);
  for (Weight d : result.dist) {
    require(d != INF, "every vertex must be reachable from source 0");
  }

  require_same_results(graph, 0);
  require_same_results(graph, 1000);
}

void test_sparse_graph_edge_floor() {
  // Requesting fewer edges than the spanning tree still yields a connected graph.
  const Graph graph = sssp::make_sparse_graph({
      .vertices = 50,
      .edges = 0,
      .max_weight = 10,
      .seed = 1,
  });

  require(graph.edge_count() == 49, "edge count should be raised to vertices-1");
  const auto result = sssp::dijkstra_binary_heap(graph, 0);
  for (Weight d : result.dist) {
    require(d != INF, "every vertex must be reachable from source 0");
  }
  require_same_results(graph, 0);
}

void test_dense_graph_connectivity_and_consistency() {
  const Graph graph = sssp::make_dense_graph({
      .vertices = 400,
      .density_percent = 25,
      .max_weight = 1'000'000,
      .seed = 77,
  });

  require(graph.vertex_count() == 400, "dense graph should have 400 vertices");
  const auto result = sssp::dijkstra_binary_heap(graph, 0);
  for (Weight d : result.dist) {
    require(d != INF, "every vertex must be reachable from source 0");
  }
  require_same_results(graph, 0);
  require_same_results(graph, 200);
}

void test_layered_graph_connectivity_and_consistency() {
  const Graph graph = sssp::make_layered_graph({
      .layers = 50,
      .width = 30,
      .degree = 3,
      .max_weight = 1'000'000,
      .seed = 31,
  });

  require(graph.vertex_count() == 1 + 50 * 30, "layered graph vertex count mismatch");
  const auto result = sssp::dijkstra_binary_heap(graph, 0);
  for (Weight d : result.dist) {
    require(d != INF, "every vertex must be reachable from super-source 0");
  }
  require_same_results(graph, 0);
}

}  // namespace

int main() {
  test_small_weighted_graph();
  test_unreachable_vertices();
  test_zero_weight_edges();
  test_cycle_graph();
  test_unit_weight_grid_graph();
  test_seeded_weighted_grid_consistency();
  test_large_weight_range_consistency();
  test_sparse_graph_connectivity_and_consistency();
  test_sparse_graph_edge_floor();
  test_dense_graph_connectivity_and_consistency();
  test_layered_graph_connectivity_and_consistency();

  std::cout << "All shortest-path tests passed.\n";
  return 0;
}

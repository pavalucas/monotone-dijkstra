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
  const auto radix = sssp::dijkstra_one_level_radix_heap(graph, source);
  require(binary.dist == radix.dist, "binary heap and radix heap returned different distances");
  require(sssp::distance_checksum(binary.dist) == sssp::distance_checksum(radix.dist),
          "binary heap and radix heap checksums differ");
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

}  // namespace

int main() {
  test_small_weighted_graph();
  test_unreachable_vertices();
  test_zero_weight_edges();
  test_cycle_graph();
  test_unit_weight_grid_graph();
  test_seeded_weighted_grid_consistency();

  std::cout << "All shortest-path tests passed.\n";
  return 0;
}

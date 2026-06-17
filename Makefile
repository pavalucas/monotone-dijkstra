CXX ?= c++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -pedantic
CPPFLAGS ?= -Iinclude

HEADERS := include/graph.hpp include/graph_generators.hpp include/graph_spec.hpp \
           include/dijkstra.hpp include/binary_heap.hpp include/radix_heap.hpp \
           include/algorithms.hpp

.PHONY: all test benchmark benchmark-grid clean

all: build/tests build/benchmark

build/tests: tests/test_shortest_paths.cpp $(HEADERS)
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_shortest_paths.cpp -o build/tests

build/benchmark: src/benchmark.cpp $(HEADERS)
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/benchmark.cpp -o build/benchmark

test: build/tests
	./build/tests

# Convenience: a single default grid benchmark to stdout.
benchmark: build/benchmark
	./build/benchmark --family grid --param rows=100 --param cols=100

# Full timestamped grid pass into results/.
benchmark-grid: build/benchmark
	./scripts/run.sh grid_sweep

clean:
	rm -rf build

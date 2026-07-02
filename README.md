# monotone-dijkstra

Experimental comparison of **monotone priority queue** implementations for
Dijkstra's single-source shortest path (SSSP) algorithm on graphs with
non-negative **integer** edge weights.

The code accompanies a course project on graph optimization based on Costa,
Castro & de Freitas, *"Exploring Monotone Priority Queues for Dijkstra
Optimization"* (RAIRO Operations Research 59, 2025), with the root algorithmic
reference Ahuja, Mehlhorn, Orlin & Tarjan, *"Faster algorithms for the shortest
path problem"* (J. ACM 37, 1990).

## Algorithms

| Priority queue | Dijkstra complexity | Status |
|---|---|---|
| Binary heap | O((m + n) log n) | implemented |
| One-level radix heap | O(m + n log C) | implemented |
| Two-level radix heap | O(m + n log C / log log C) | implemented |
| Radix + Fibonacci heap | O(m + n √log C) | not implemented (theoretical) |

`C` is the maximum edge weight.

## Where the code is

All the implementation lives under [`include/`](include/) (header-only) and
[`src/`](src/):

| File | Contents |
|---|---|
| [`include/radix_heap.hpp`](include/radix_heap.hpp) | **one-level and two-level radix heaps** — the core of this project |
| [`include/binary_heap.hpp`](include/binary_heap.hpp) | baseline binary heap (`std::priority_queue` wrapper) |
| [`include/dijkstra.hpp`](include/dijkstra.hpp) | generic Dijkstra driver with lazy deletion, templated on the queue |
| [`include/graph.hpp`](include/graph.hpp) | directed weighted graph (adjacency list) |
| [`include/graph_generators.hpp`](include/graph_generators.hpp) | grid, sparse, dense and layered graph generators |
| [`include/graph_spec.hpp`](include/graph_spec.hpp) | graph-family dispatch (`--family`/`--param` → generator) |
| [`include/algorithms.hpp`](include/algorithms.hpp) | algorithm registry (name → SSSP function) |
| [`src/benchmark.cpp`](src/benchmark.cpp) | CLI benchmark: algorithms × graph family → CSV |
| [`tests/test_shortest_paths.cpp`](tests/test_shortest_paths.cpp) | unit tests |

## Graph families

The benchmark generates four synthetic families (selected with `--family`):

| Family | Description |
|---|---|
| `grid` | bidirectional 2D mesh (planar, sparse, `m ≈ 4n`) |
| `sparse` | random sparse graph: spanning tree + random edges (`m ≈ 3n`) |
| `dense` | Erdős–Rényi with a given density (`m ≈ n²`) |
| `layered` | layered DAG from a super-source (`L` layers of width `W`) |

## Build

```bash
make          # builds build/tests and build/benchmark
make test     # compiles and runs the unit tests
make clean    # removes build/
```

Requires a C++20 compiler. Override `CXX` / `CXXFLAGS` as needed (default `-O2`).

## Running the benchmark

The benchmark is registry-driven: it runs any selection of algorithms over a
chosen graph family.

```bash
build/benchmark --family grid --param rows=300 --param cols=300 \
    --max-weight 1000 --seed 1 --sources 10 --repetitions 3 --algorithms all

build/benchmark --list-algorithms
```

It writes CSV (one `run` row per algorithm/source/repetition, plus per-algorithm
`summary` rows with the median runtime). All algorithms are cross-checked against
a shared distance checksum per source.

## Reproducible experiments

Experiments are defined under [`experiments/`](experiments/) and run via:

```bash
./scripts/run.sh grid_sweep [optional-label]
```

This builds the benchmark and writes a self-contained, timestamped folder
`results/<UTC-timestamp>-grid_sweep[-label]/` with `results.csv` and a
`manifest.txt` recording the host, compiler, flags, and algorithm list.

Two experiments are provided: `grid_sweep` (grid sizes × weight ranges) and
`full_sweep` (all algorithms × all four families × `C ∈ {10, 10³, 10⁶, 10⁹}`).
Summarize a run's `results.csv` into median tables with
[`scripts/analyze_results.py`](scripts/analyze_results.py) (or a bar chart with
[`scripts/plot_results.py`](scripts/plot_results.py)).

## Extending

- **New algorithm variant** — implement `dijkstra_with_queue<YourQueue>` in
  [`include/dijkstra.hpp`](include/dijkstra.hpp) (a queue exposing
  `push(Weight, Vertex)`, `pop_min() -> {Weight, Vertex}`, `empty()`), then add
  one line to `algorithm_registry()` in
  [`include/algorithms.hpp`](include/algorithms.hpp).
- **New graph family** — add a generator in
  [`include/graph_generators.hpp`](include/graph_generators.hpp) and a branch to
  `build_graph` in [`include/graph_spec.hpp`](include/graph_spec.hpp).

## Layout

```
include/        headers (graph, queues, Dijkstra driver, registries)
src/            benchmark.cpp — generic CLI benchmark
scripts/        run.sh, lib.sh, analyze_results.py, plot_results.py
experiments/    grid_sweep.sh, full_sweep.sh
tests/          unit tests (no framework)
```

## License

MIT — see [LICENSE](LICENSE).

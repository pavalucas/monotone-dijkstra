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
| Two-level radix heap | O(m + n log C / log log C) | planned |
| Radix + Fibonacci heap | O(m + n √log C) | planned |

`C` is the maximum edge weight.

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
include/      headers (graph, queues, Dijkstra driver, registries)
src/          benchmark.cpp — generic CLI benchmark
scripts/      run.sh (timestamped runner) + lib.sh
experiments/  experiment definitions
tests/        unit tests (no framework)
```

## License

MIT — see [LICENSE](LICENSE).

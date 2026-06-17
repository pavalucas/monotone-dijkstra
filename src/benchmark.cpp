#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "algorithms.hpp"
#include "dijkstra.hpp"
#include "graph_spec.hpp"

namespace {

using Clock = std::chrono::steady_clock;

struct Config {
  sssp::GraphSpec graph;
  int sources = 5;
  int repetitions = 3;
  std::vector<std::string> algorithms;  // empty => all registered
  std::string out_path;
};

struct RunRow {
  std::string algorithm;
  int source = 0;
  int repetition = 0;
  double runtime_ms = 0.0;
  std::uint64_t pushes = 0;
  std::uint64_t pops = 0;
  std::uint64_t stale_pops = 0;
  std::uint64_t relax_attempts = 0;
  std::uint64_t relax_successes = 0;
  std::uint64_t checksum = 0;
};

void print_usage(const char* program) {
  std::cerr
      << "Usage: " << program << " [options]\n\n"
      << "Options:\n"
      << "  --family NAME         Graph family. Default: grid\n"
      << "  --param key=value     Graph parameter (repeatable), e.g. --param rows=100\n"
      << "  --max-weight C        Maximum edge weight. Default: 1000\n"
      << "  --seed S              Random seed. Default: 1\n"
      << "  --sources K           Number of random sources. Default: 5\n"
      << "  --repetitions R       Repetitions per source. Default: 3\n"
      << "  --algorithms LIST     Comma-separated names or 'all'. Default: all\n"
      << "  --out FILE            Write CSV output to FILE instead of stdout\n"
      << "  --list-algorithms     Print registered algorithm names and exit\n"
      << "  --help                Show this message\n";
}

std::vector<std::string> split_csv(const std::string& value) {
  std::vector<std::string> parts;
  std::stringstream stream(value);
  std::string item;
  while (std::getline(stream, item, ',')) {
    if (!item.empty()) {
      parts.push_back(item);
    }
  }
  return parts;
}

Config parse_args(int argc, char** argv) {
  Config config;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    auto require_value = [&](const std::string& name) -> std::string {
      if (i + 1 >= argc) {
        throw std::invalid_argument("missing value for " + name);
      }
      return argv[++i];
    };

    if (arg == "--help") {
      print_usage(argv[0]);
      std::exit(0);
    } else if (arg == "--list-algorithms") {
      for (const sssp::AlgorithmEntry& entry : sssp::algorithm_registry()) {
        std::cout << entry.name << '\n';
      }
      std::exit(0);
    } else if (arg == "--family") {
      config.graph.family = require_value(arg);
    } else if (arg == "--param") {
      const std::string kv = require_value(arg);
      const auto eq = kv.find('=');
      if (eq == std::string::npos) {
        throw std::invalid_argument("--param expects key=value, got: " + kv);
      }
      config.graph.params[kv.substr(0, eq)] = kv.substr(eq + 1);
    } else if (arg == "--max-weight") {
      config.graph.max_weight = std::stoull(require_value(arg));
    } else if (arg == "--seed") {
      config.graph.seed = std::stoull(require_value(arg));
    } else if (arg == "--sources") {
      config.sources = std::stoi(require_value(arg));
    } else if (arg == "--repetitions") {
      config.repetitions = std::stoi(require_value(arg));
    } else if (arg == "--algorithms") {
      const std::string value = require_value(arg);
      if (value != "all") {
        config.algorithms = split_csv(value);
      }
    } else if (arg == "--out") {
      config.out_path = require_value(arg);
    } else {
      throw std::invalid_argument("unknown option: " + arg);
    }
  }

  if (config.graph.max_weight == 0) {
    throw std::invalid_argument("max-weight must be positive");
  }
  if (config.sources <= 0) {
    throw std::invalid_argument("sources must be positive");
  }
  if (config.repetitions <= 0) {
    throw std::invalid_argument("repetitions must be positive");
  }
  return config;
}

std::vector<const sssp::AlgorithmEntry*> resolve_algorithms(const Config& config) {
  std::vector<const sssp::AlgorithmEntry*> selected;
  if (config.algorithms.empty()) {
    for (const sssp::AlgorithmEntry& entry : sssp::algorithm_registry()) {
      selected.push_back(&entry);
    }
    return selected;
  }
  for (const std::string& name : config.algorithms) {
    const sssp::AlgorithmEntry* entry = sssp::find_algorithm(name);
    if (entry == nullptr) {
      throw std::invalid_argument("unknown algorithm: " + name +
                                  " (use --list-algorithms)");
    }
    selected.push_back(entry);
  }
  return selected;
}

std::vector<sssp::Vertex> make_sources(int n, int count, std::uint64_t seed) {
  std::mt19937_64 rng(seed ^ 0x9e3779b97f4a7c15ULL);
  std::uniform_int_distribution<int> dist(0, n - 1);
  std::vector<sssp::Vertex> sources;
  sources.reserve(count);
  for (int i = 0; i < count; ++i) {
    sources.push_back(dist(rng));
  }
  return sources;
}

RunRow measure_run(const sssp::AlgorithmEntry& algorithm, const sssp::Graph& graph,
                   int source, int repetition) {
  const auto start = Clock::now();
  const sssp::ShortestPathResult result = algorithm.run(graph, source);
  const auto end = Clock::now();

  RunRow row;
  row.algorithm = algorithm.name;
  row.source = source;
  row.repetition = repetition;
  row.runtime_ms = std::chrono::duration<double, std::milli>(end - start).count();
  row.pushes = result.stats.pushes;
  row.pops = result.stats.pops;
  row.stale_pops = result.stats.stale_pops;
  row.relax_attempts = result.stats.relax_attempts;
  row.relax_successes = result.stats.relax_successes;
  row.checksum = sssp::distance_checksum(result.dist);
  return row;
}

double median(std::vector<double> values) {
  if (values.empty()) {
    return 0.0;
  }
  std::sort(values.begin(), values.end());
  const std::size_t mid = values.size() / 2;
  if (values.size() % 2 == 1) {
    return values[mid];
  }
  return (values[mid - 1] + values[mid]) / 2.0;
}

double average(const std::vector<double>& values) {
  if (values.empty()) {
    return 0.0;
  }
  double sum = 0.0;
  for (double value : values) {
    sum += value;
  }
  return sum / static_cast<double>(values.size());
}

void write_header(std::ostream& out) {
  out << "row_type,graph_family,graph_params,n,m,C,seed,algorithm,source,repetition,"
      << "runtime_ms,pushes,pops,stale_pops,relax_attempts,relax_successes,checksum,status\n";
}

void write_prefix(std::ostream& out, const Config& config, const std::string& params,
                  int n, std::uint64_t m) {
  out << config.graph.family << ',' << '"' << params << '"' << ',' << n << ',' << m << ','
      << config.graph.max_weight << ',' << config.graph.seed << ',';
}

void write_run(std::ostream& out, const Config& config, const std::string& params, int n,
               std::uint64_t m, const RunRow& row, const std::string& status) {
  out << "run,";
  write_prefix(out, config, params, n, m);
  out << row.algorithm << ',' << row.source << ',' << row.repetition << ',' << std::fixed
      << std::setprecision(6) << row.runtime_ms << ',' << row.pushes << ',' << row.pops << ','
      << row.stale_pops << ',' << row.relax_attempts << ',' << row.relax_successes << ','
      << row.checksum << ',' << status << '\n';
}

void write_summary(std::ostream& out, const Config& config, const std::string& params, int n,
                   std::uint64_t m, const std::string& algorithm,
                   const std::vector<RunRow>& rows) {
  std::vector<double> times;
  for (const RunRow& row : rows) {
    if (row.algorithm == algorithm) {
      times.push_back(row.runtime_ms);
    }
  }
  if (times.empty()) {
    return;
  }
  const auto [min_it, max_it] = std::minmax_element(times.begin(), times.end());
  out << "summary,";
  write_prefix(out, config, params, n, m);
  out << algorithm << ",ALL,ALL," << std::fixed << std::setprecision(6) << median(times)
      << ",,,,,,,median_ms;avg_ms=" << average(times) << ";min_ms=" << *min_it
      << ";max_ms=" << *max_it << '\n';
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Config config = parse_args(argc, argv);
    const std::vector<const sssp::AlgorithmEntry*> algorithms = resolve_algorithms(config);

    const sssp::Graph graph = sssp::build_graph(config.graph);
    const std::string params = sssp::graph_params_string(config.graph);
    const int n = graph.vertex_count();
    const std::uint64_t m = graph.edge_count();
    const std::vector<sssp::Vertex> sources = make_sources(n, config.sources, config.graph.seed);

    std::ofstream file_out;
    std::ostream* out = &std::cout;
    if (!config.out_path.empty()) {
      file_out.open(config.out_path);
      if (!file_out) {
        throw std::runtime_error("could not open output file: " + config.out_path);
      }
      out = &file_out;
    }

    write_header(*out);
    std::vector<RunRow> all_rows;

    for (sssp::Vertex source : sources) {
      std::uint64_t expected_checksum = 0;
      bool has_expected_checksum = false;

      for (int repetition = 0; repetition < config.repetitions; ++repetition) {
        for (const sssp::AlgorithmEntry* algorithm : algorithms) {
          RunRow row = measure_run(*algorithm, graph, source, repetition);
          if (!has_expected_checksum) {
            expected_checksum = row.checksum;
            has_expected_checksum = true;
          }
          const std::string status =
              row.checksum == expected_checksum ? "ok" : "checksum_mismatch";
          write_run(*out, config, params, n, m, row, status);
          all_rows.push_back(std::move(row));
        }
      }
    }

    for (const sssp::AlgorithmEntry* algorithm : algorithms) {
      write_summary(*out, config, params, n, m, algorithm->name, all_rows);
    }
  } catch (const std::exception& error) {
    std::cerr << "error: " << error.what() << "\n\n";
    print_usage(argv[0]);
    return 1;
  }

  return 0;
}

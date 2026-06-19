#pragma once

#include <map>
#include <stdexcept>
#include <string>

#include "graph.hpp"
#include "graph_generators.hpp"

namespace sssp {

// A graph family plus its free-form parameters. To add a family, add a branch
// to `build_graph` that reads the parameters it needs from `params`.
struct GraphSpec {
  std::string family = "grid";
  std::map<std::string, std::string> params;
  Weight max_weight = 1000;
  std::uint64_t seed = 1;
};

namespace detail {

inline int param_int(const std::map<std::string, std::string>& params,
                     const std::string& key, int fallback) {
  const auto it = params.find(key);
  return it == params.end() ? fallback : std::stoi(it->second);
}

inline std::uint64_t param_u64(const std::map<std::string, std::string>& params,
                               const std::string& key, std::uint64_t fallback) {
  const auto it = params.find(key);
  return it == params.end() ? fallback : std::stoull(it->second);
}

inline bool param_bool(const std::map<std::string, std::string>& params,
                       const std::string& key, bool fallback) {
  const auto it = params.find(key);
  if (it == params.end()) {
    return fallback;
  }
  return it->second == "true" || it->second == "1";
}

}  // namespace detail

inline Graph build_graph(const GraphSpec& spec) {
  if (spec.family == "grid") {
    return make_grid_graph({
        .rows = detail::param_int(spec.params, "rows", 100),
        .cols = detail::param_int(spec.params, "cols", 100),
        .max_weight = spec.max_weight,
        .seed = spec.seed,
        .bidirectional = detail::param_bool(spec.params, "bidirectional", true),
    });
  }
  if (spec.family == "sparse") {
    const int vertices = detail::param_int(spec.params, "vertices", 100000);
    return make_sparse_graph({
        .vertices = vertices,
        .edges = detail::param_u64(spec.params, "edges",
                                   static_cast<std::uint64_t>(vertices) * 3),
        .max_weight = spec.max_weight,
        .seed = spec.seed,
    });
  }
  throw std::invalid_argument("unknown graph family: " + spec.family);
}

inline std::string graph_params_string(const GraphSpec& spec) {
  std::string out;
  for (const auto& [key, value] : spec.params) {
    if (!out.empty()) {
      out += ';';
    }
    out += key + "=" + value;
  }
  return out;
}

}  // namespace sssp

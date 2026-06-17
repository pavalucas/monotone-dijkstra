# Helpers for experiment definitions. Sourced by scripts/run.sh, which exports
# BENCH (path to the benchmark binary) and RESULTS_CSV (target file).

# bench <benchmark args...> — run one benchmark invocation and append its rows
# to RESULTS_CSV, keeping a single header line across invocations.
bench() {
  if [ -s "$RESULTS_CSV" ]; then
    "$BENCH" "$@" | grep -v '^row_type' >> "$RESULTS_CSV"
  else
    "$BENCH" "$@" >> "$RESULTS_CSV"
  fi
}

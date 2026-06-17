#!/usr/bin/env bash
# Run a named experiment into a timestamped results folder.
#
#   scripts/run.sh <experiment> [label]
#
# <experiment> is a file experiments/<experiment>.sh that calls `bench ...`
# (see scripts/lib.sh). Output goes to results/<UTC-timestamp>-<experiment>[-label]/
# containing results.csv plus manifest.txt describing how it was produced.
set -euo pipefail

exp="${1:?usage: scripts/run.sh <experiment> [label]}"
label="${2:-}"

root="$(cd "$(dirname "$0")/.." && pwd)"
expfile="$root/experiments/$exp.sh"
if [ ! -f "$expfile" ]; then
  echo "no such experiment: $expfile" >&2
  echo "available:" >&2
  ls "$root/experiments" 2>/dev/null | sed 's/\.sh$//;s/^/  /' >&2 || true
  exit 1
fi

ts="$(date -u +%Y%m%dT%H%M%SZ)"
name="$ts-$exp${label:+-$label}"
outdir="$root/results/$name"
mkdir -p "$outdir"

export BENCH="$root/build/benchmark"
export RESULTS_CSV="$outdir/results.csv"

make -C "$root" build/benchmark >/dev/null

{
  echo "experiment:    $exp"
  echo "label:         ${label:-(none)}"
  echo "timestamp_utc: $ts"
  echo "host:          $(hostname)"
  echo "os:            $(uname -srm)"
  echo "compiler:      $(${CXX:-c++} --version | head -1)"
  echo "cxxflags:      ${CXXFLAGS:--std=c++20 -O2 -Wall -Wextra -pedantic (default)}"
  echo "algorithms:    $("$BENCH" --list-algorithms | paste -sd, -)"
} > "$outdir/manifest.txt"

# shellcheck source=scripts/lib.sh
source "$root/scripts/lib.sh"
# shellcheck source=/dev/null
source "$expfile"

echo "results -> $RESULTS_CSV"
echo "manifest -> $outdir/manifest.txt"

#!/usr/bin/env python3
"""Summarize a benchmark results.csv: median runtime per algorithm across graph
families and weight ranges (C). Standard library only.

Usage:
    scripts/analyze_results.py [results.csv] [--drop-c C,...] [--keep-blocks fam:n,...]

With no argument, the most recent results/*/results.csv is used. Writes a
markdown table next to the CSV and LaTeX tables into article/. The console and
markdown summaries always show the full data; --drop-c and --keep-blocks only
filter the LaTeX tables (used to trim the concise Revista article -- the course
version regenerates full tables by omitting the flags).
"""
import argparse
import csv
import glob
import os
import sys

ALGOS = ["binary_heap", "one_level_radix_heap", "two_level_radix_heap"]
# Presentation order for graph families (matches the article text); unknown
# families sort after these, alphabetically.
FAMILY_ORDER = {"grid": 0, "sparse": 1, "dense": 2, "layered": 3}


def family_rank(family):
    return (FAMILY_ORDER.get(family, len(FAMILY_ORDER)), family)
ALGO_LABEL = {
    "binary_heap": "binary",
    "one_level_radix_heap": "radix-1",
    "two_level_radix_heap": "radix-2",
}


def latest_csv():
    candidates = sorted(glob.glob("results/*/results.csv"))
    if not candidates:
        sys.exit("no results/*/results.csv found")
    return candidates[-1]


def load(path):
    # rows[(family, n, m, C)][algorithm] = median_ms
    rows = {}
    with open(path, newline="") as fh:
        for r in csv.reader(fh):
            if not r or r[0] != "summary":
                continue
            family, n, m, C, algo, median = r[1], int(r[3]), int(r[4]), int(r[5]), r[7], float(r[10])
            rows.setdefault((family, n, m, C), {})[algo] = median
    return rows


def load_opcounts(path):
    # Operation counts are identical across queue types (same settle order), so
    # read binary_heap run rows. Averaged per (family, n) over all C and sources.
    acc = {}
    with open(path, newline="") as fh:
        for r in csv.reader(fh):
            if not r or r[0] != "run" or r[7] != "binary_heap":
                continue
            family, n = r[1], int(r[3])
            a = acc.setdefault((family, n), {"m": 0, "pushes": 0, "pops": 0, "stale": 0, "cnt": 0})
            a["m"] += int(r[4])
            a["pushes"] += int(r[11])
            a["pops"] += int(r[12])
            a["stale"] += int(r[13])
            a["cnt"] += 1
    return acc


def fmt_key(family, n, m):
    return f"{family} (n={n:,}, m={m:,})"


def fmt_n(n):
    return f"{n // 1000}k" if n >= 1000 else str(n)


def fmt_sci(x):
    # LaTeX scientific notation with a decimal comma, e.g. 2{,}50\times10^{5},
    # matching the hand-written generator-parameter table in the course article.
    v = float(x)
    e = 0
    while v >= 10:
        v /= 10
        e += 1
    mant = f"{v:.2f}".replace(".", "{,}")
    return f"${mant}\\times10^{{{e}}}$"


def fmt_c(C):
    # Scientific notation for the C column (weights are powers of ten here).
    import math
    if C <= 0:
        return str(C)
    e = round(math.log10(C))
    if 10 ** e == C:
        return "$10$" if e == 1 else f"$10^{{{e}}}$"
    mant = C / 10 ** e
    return f"${mant:.2f}".replace(".", "{,}") + f"\\times 10^{{{e}}}$"


def winner(medians):
    best = min(medians, key=medians.get)
    return best


def emit_console(rows):
    keys = sorted(rows, key=lambda k: (k[0], k[1], k[3]))
    last_group = None
    for family, n, m, C in keys:
        group = (family, n, m)
        if group != last_group:
            print(f"\n{fmt_key(family, n, m)}")
            print(f"  {'C':>12}  {'binary':>9}  {'radix-1':>9}  {'radix-2':>9}  winner   radix speedup")
            last_group = group
        med = rows[(family, n, m, C)]
        b = med.get("binary_heap")
        best_radix = min(med.get("one_level_radix_heap", 1e9), med.get("two_level_radix_heap", 1e9))
        speedup = b / best_radix if best_radix else float("nan")
        win = ALGO_LABEL[winner(med)]
        print(f"  {C:>12,}  {med['binary_heap']:>9.3f}  "
              f"{med['one_level_radix_heap']:>9.3f}  {med['two_level_radix_heap']:>9.3f}  "
              f"{win:>6}   {speedup:>6.2f}x")


def emit_markdown(rows, path):
    keys = sorted(rows, key=lambda k: (k[0], k[1], k[3]))
    lines = ["# Results summary (median runtime, ms)\n"]
    last_group = None
    for family, n, m, C in keys:
        group = (family, n, m)
        if group != last_group:
            lines.append(f"\n## {fmt_key(family, n, m)}\n")
            lines.append("| C | binary | radix-1 | radix-2 | winner |")
            lines.append("|---:|---:|---:|---:|:--|")
            last_group = group
        med = rows[(family, n, m, C)]
        lines.append(f"| {C:,} | {med['binary_heap']:.3f} | {med['one_level_radix_heap']:.3f} "
                     f"| {med['two_level_radix_heap']:.3f} | {ALGO_LABEL[winner(med)]} |")
    with open(path, "w") as fh:
        fh.write("\n".join(lines) + "\n")
    return path


def emit_latex(rows, path, drop_c=(), keep_blocks=None):
    keys = [k for k in sorted(rows, key=lambda k: (family_rank(k[0]), k[1], k[3]))
            if k[3] not in drop_c and (keep_blocks is None or (k[0], k[1]) in keep_blocks)]
    out = [
        "% Generated by scripts/analyze_results.py -- do not edit by hand.",
        "\\begin{table}[H]",
        "    \\centering",
        "    \\caption{Tempo mediano de execução (ms) por fila de prioridade. "
        "Melhor valor por linha em \\textbf{negrito}.}",
        "    \\label{tab:resultados}",
        "    \\footnotesize",
        "    \\begin{tabular}{@{}llrrr@{}}",
        "        \\toprule",
        "        Família & $C$ & binary & radix-1 & radix-2 \\\\",
        "        \\midrule",
    ]
    last_block = None
    for family, n, m, C in keys:
        med = rows[(family, n, m, C)]
        best = winner(med)
        cells = []
        for a in ALGOS:
            v = f"{med[a]:.2f}".replace(".", "{,}")
            cells.append(f"\\textbf{{{v}}}" if a == best else v)
        block = (family, n)
        if block != last_block:
            if last_block is not None:
                out.append("        \\midrule")
            fam_cell = f"{family} ($n{{=}}{fmt_sci(n).strip('$')}$)"
        else:
            fam_cell = ""
        last_block = block
        out.append(f"        {fam_cell} & {fmt_c(C)} & {cells[0]} & {cells[1]} & {cells[2]} \\\\")
    out += ["        \\bottomrule", "    \\end{tabular}", "\\end{table}", ""]
    with open(path, "w") as fh:
        fh.write("\n".join(out))
    return path


def emit_opcounts_latex(acc, path, keep_blocks=None):
    out = [
        "% Generated by scripts/analyze_results.py -- do not edit by hand.",
        "\\begin{table}[H]",
        "    \\centering",
        "    \\caption{Contadores de operação por família (média sobre as execuções; "
        "idênticos entre as filas). push/$n$ = relaxações bem-sucedidas por vértice; "
        "\\textit{stale}\\,\\% = fração de remoções obsoletas (\\textit{lazy deletion}).}",
        "    \\label{tab:opcounts}",
        "    \\footnotesize",
        "    \\begin{tabular}{@{}lrrrr@{}}",
        "        \\toprule",
        "        Família & $n$ & $m$ & push/$n$ & \\textit{stale}\\,\\% \\\\",
        "        \\midrule",
    ]
    for (family, n) in sorted(acc, key=lambda k: (family_rank(k[0]), k[1])):
        if keep_blocks is not None and (family, n) not in keep_blocks:
            continue
        a = acc[(family, n)]
        m = round(a["m"] / a["cnt"])
        push_per_n = (a["pushes"] / a["cnt"]) / n
        stale_pct = 100 * a["stale"] / a["pops"] if a["pops"] else 0
        push_cell = f"{push_per_n:.2f}".replace(".", "{,}")
        stale_cell = f"{stale_pct:.1f}".replace(".", "{,}")
        out.append(f"        {family} & {fmt_sci(n)} & {fmt_sci(m)} & {push_cell} & {stale_cell} \\\\")
    out += ["        \\bottomrule", "    \\end{tabular}", "\\end{table}", ""]
    with open(path, "w") as fh:
        fh.write("\n".join(out))
    return path


def parse_blocks(spec):
    if not spec:
        return None
    out = set()
    for tok in spec.split(","):
        fam, _, n = tok.partition(":")
        out.add((fam, int(n)))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", nargs="?", default=None)
    ap.add_argument("--drop-c", default="", help="comma-separated C values to omit from the LaTeX results table")
    ap.add_argument("--keep-blocks", default="", help="comma-separated family:n blocks to keep in the LaTeX tables (others dropped)")
    ap.add_argument("--out-dir", default="article", help="folder for the LaTeX tables (e.g. article-course)")
    args = ap.parse_args()

    csv_path = args.csv if args.csv else latest_csv()
    drop_c = {int(x) for x in args.drop_c.split(",") if x.strip()}
    keep_blocks = parse_blocks(args.keep_blocks)

    rows = load(csv_path)
    print(f"source: {csv_path}")
    emit_console(rows)
    md = emit_markdown(rows, os.path.join(os.path.dirname(csv_path), "summary.md"))
    tex = emit_latex(rows, os.path.join(args.out_dir, "results_table.tex"), drop_c, keep_blocks)
    optex = emit_opcounts_latex(load_opcounts(csv_path), os.path.join(args.out_dir, "opcounts_table.tex"), keep_blocks)
    print(f"\nwrote {md}")
    print(f"wrote {tex}")
    print(f"wrote {optex}")


if __name__ == "__main__":
    main()

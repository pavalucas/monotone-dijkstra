#!/usr/bin/env python3
"""Bar chart of median runtime per algorithm vs weight range C, one panel per
graph family.

Usage:
    scripts/plot_results.py [results.csv]

Uses matplotlib if available (writes results/<run>/chart.png and .pdf); otherwise
falls back to a dependency-free SVG (results/<run>/chart.svg) so a figure is
always produced.
"""
import csv
import glob
import os
import sys

ALGOS = ["binary_heap", "one_level_radix_heap", "two_level_radix_heap"]
LABELS = ["binary", "radix-1", "radix-2"]
COLORS = ["#4477aa", "#ee6677", "#228833"]


def latest_csv():
    c = sorted(glob.glob("results/*/results.csv"))
    if not c:
        sys.exit("no results/*/results.csv found")
    return c[-1]


def load(path):
    # data[family][C][algo] = median ms ; keep one representative size per family
    data, size = {}, {}
    with open(path, newline="") as fh:
        for r in csv.reader(fh):
            if not r or r[0] != "summary":
                continue
            family, n, m, C, algo, med = r[1], int(r[3]), int(r[4]), int(r[5]), r[7], float(r[10])
            # pick the largest n seen per family for the figure
            if family not in size or n >= size[family][0]:
                if family not in size or n > size[family][0]:
                    data.setdefault(family, {}).clear()
                size[family] = (n, m)
            data.setdefault(family, {}).setdefault(C, {})[algo] = med
    return data, size


def cvals(fam_data):
    return sorted(fam_data)


def plot_matplotlib(data, size, outbase):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    families = sorted(data)
    fig, axes = plt.subplots(1, len(families), figsize=(4 * len(families), 3.2), squeeze=False)
    for ax, fam in zip(axes[0], families):
        Cs = cvals(data[fam])
        x = range(len(Cs))
        w = 0.26
        for j, (algo, lab, col) in enumerate(zip(ALGOS, LABELS, COLORS)):
            ys = [data[fam][C].get(algo, 0) for C in Cs]
            ax.bar([xi + (j - 1) * w for xi in x], ys, width=w, label=lab, color=col)
        ax.set_yscale("log")
        ax.set_xticks(list(x))
        ax.set_xticklabels([f"$10^{{{len(str(C)) - 1}}}$" for C in Cs])
        ax.set_title(f"{fam}\n(n={size[fam][0]:,})", fontsize=9)
        ax.set_xlabel("C")
        ax.grid(axis="y", ls=":", alpha=0.5)
    axes[0][0].set_ylabel("median runtime (ms)")
    axes[0][-1].legend(fontsize=8)
    fig.tight_layout()
    fig.savefig(outbase + ".png", dpi=150)
    fig.savefig(outbase + ".pdf")
    return [outbase + ".png", outbase + ".pdf"]


def plot_svg(data, size, outpath):
    families = sorted(data)
    panel_w, panel_h, pad = 300, 230, 50
    plot_h = panel_h - 2 * pad + 40
    width = panel_w * len(families)
    height = panel_h + 40
    maxv = max(v for fam in data.values() for c in fam.values() for v in c.values())

    import math
    def y_of(v):  # log scale
        lo, hi = math.log10(0.1), math.log10(maxv * 1.3)
        t = (math.log10(max(v, 0.1)) - lo) / (hi - lo)
        return pad + plot_h * (1 - t)

    svg = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
           f'font-family="sans-serif" font-size="11">']
    svg.append(f'<rect width="{width}" height="{height}" fill="white"/>')
    for pi, fam in enumerate(families):
        ox = pi * panel_w
        Cs = cvals(data[fam])
        svg.append(f'<text x="{ox + panel_w/2}" y="16" text-anchor="middle" '
                   f'font-weight="bold">{fam} (n={size[fam][0]:,})</text>')
        svg.append(f'<line x1="{ox+pad}" y1="{pad+plot_h}" x2="{ox+panel_w-10}" '
                   f'y2="{pad+plot_h}" stroke="#888"/>')
        gw = (panel_w - pad - 20) / len(Cs)
        bw = gw / 4
        for gi, C in enumerate(Cs):
            gx = ox + pad + gi * gw + bw / 2
            for j, (algo, col) in enumerate(zip(ALGOS, COLORS)):
                v = data[fam][C].get(algo, 0)
                yb = y_of(v)
                svg.append(f'<rect x="{gx + j*bw:.1f}" y="{yb:.1f}" width="{bw-1:.1f}" '
                           f'height="{pad+plot_h-yb:.1f}" fill="{col}"/>')
            svg.append(f'<text x="{gx + 1.5*bw:.1f}" y="{pad+plot_h+14}" '
                       f'text-anchor="middle" font-size="9">1e{len(str(C))-1}</text>')
    # legend
    for j, (lab, col) in enumerate(zip(LABELS, COLORS)):
        lx = 10 + j * 90
        svg.append(f'<rect x="{lx}" y="{height-16}" width="11" height="11" fill="{col}"/>')
        svg.append(f'<text x="{lx+15}" y="{height-6}">{lab}</text>')
    svg.append("</svg>")
    with open(outpath, "w") as fh:
        fh.write("\n".join(svg))
    return [outpath]


def main():
    csv_path = sys.argv[1] if len(sys.argv) > 1 else latest_csv()
    data, size = load(csv_path)
    outdir = os.path.dirname(csv_path)
    try:
        import matplotlib  # noqa: F401
        written = plot_matplotlib(data, size, os.path.join(outdir, "chart"))
    except ImportError:
        written = plot_svg(data, size, os.path.join(outdir, "chart.svg"))
        print("matplotlib not found — wrote SVG fallback")
    for w in written:
        print("wrote", w)


if __name__ == "__main__":
    main()

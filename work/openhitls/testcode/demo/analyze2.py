#!/usr/bin/env python3
"""
SM4 Performance Analysis & Visualization
=========================================
Reads sm4_results.csv produced by sm4_benchmark and generates
comprehensive comparison plots.

Usage:
    python3 analyze.py [csv_path] [--outdir figures/]

Default:
    csv_path = ./sm4_results.csv
    outdir   = ./figures/
"""

import csv
import os
import sys
import math
import argparse
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

# ============================================================================
# Configuration
# ============================================================================
plt.rcParams.update({
    "figure.dpi": 150,
    "savefig.dpi": 300,
    "savefig.bbox": "tight",
    "font.size": 11,
    "axes.titlesize": 13,
    "axes.labelsize": 11,
    "legend.fontsize": 9,
    "xtick.labelsize": 9,
    "ytick.labelsize": 9,
})

# Color palette — consistent across all plots
SCHEME_COLORS = {
    "default_loop":  "#7f7f7f",   # gray — baseline
    "unrolled":      "#1f77b4",   # blue
    "reduced_dep":   "#ff7f0e",   # orange
    "xbox":          "#2ca02c",   # green
    "xbox_merged":   "#d62728",   # red
    "zbb_default":   "#9467bd",   # purple
    "zbb_xbox":      "#8c564b",   # brown
}

# Display-friendly short names
SCHEME_LABELS = {
    "default_loop":  "default_loop\n(baseline)",
    "unrolled":      "unrolled",
    "reduced_dep":   "reduced_dep",
    "xbox":          "xbox",
    "xbox_merged":   "xbox_merged",
    "zbb_default":   "zbb_default\n(+Zbb ISA)",
    "zbb_xbox":      "zbb_xbox\n(+Zbb ISA)",
}

# Markers for line plots
SCHEME_MARKERS = {
    "default_loop":  "s",
    "unrolled":      "o",
    "reduced_dep":   "D",
    "xbox":          "^",
    "xbox_merged":   "v",
    "zbb_default":   "p",
    "zbb_xbox":      "*",
}


# ============================================================================
# Data loading
# ============================================================================
def load_data(csv_path):
    """
    Parse the benchmark CSV into a nested dict:
        data[scheme][data_size_bytes] = {
            "label": str, "elapsed_us": float,
            "throughput_mbps": float, "speedup": float
        }
    Also returns lists: schemes (in order), sizes (in order)
    """
    d = defaultdict(dict)
    scheme_order = []
    size_order = []
    seen_schemes = set()
    seen_sizes = set()

    with open(csv_path, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            scheme = row["scheme"].strip()
            size_bytes = int(row["data_size"])
            label = row["data_label"].strip()
            elapsed = float(row["elapsed_us"])
            throughput = float(row["throughput_mbps"])
            speedup = float(row["speedup"])

            d[scheme][size_bytes] = {
                "label": label,
                "elapsed_us": elapsed,
                "throughput_mbps": throughput,
                "speedup": speedup,
            }

            if scheme not in seen_schemes:
                scheme_order.append(scheme)
                seen_schemes.add(scheme)
            if size_bytes not in seen_sizes:
                size_order.append(size_bytes)
                seen_sizes.add(size_bytes)

    return dict(d), scheme_order, sorted(size_order)


# ============================================================================
# Helpers
# ============================================================================
def size_label(size_bytes):
    """Convert bytes to a compact label like '1K', '16K', '1M', '16M'."""
    if size_bytes >= 1048576:
        val = size_bytes // 1048576
        return f"{val}M"
    elif size_bytes >= 1024:
        val = size_bytes // 1024
        return f"{val}K"
    else:
        return str(size_bytes)


def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


# ============================================================================
# Plotting functions
# ============================================================================

def plot_throughput_bars(data, schemes, sizes, outdir):
    """Grouped bar chart: throughput (MB/s) per scheme per data size."""
    fig, ax = plt.subplots(figsize=(14, 6))
    x = np.arange(len(sizes))
    width = 0.11
    n_schemes = len(schemes)

    for i, scheme in enumerate(schemes):
        vals = [data[scheme][s]["throughput_mbps"] for s in sizes]
        offset = (i - (n_schemes - 1) / 2) * width
        ax.bar(x + offset, vals, width,
               color=SCHEME_COLORS.get(scheme, "#333333"),
               label=SCHEME_LABELS.get(scheme, scheme),
               edgecolor="white", linewidth=0.3)

    ax.set_xlabel("Data Size")
    ax.set_ylabel("Throughput (MB/s)")
    ax.set_title("SM4 Throughput Comparison Across Optimization Schemes")
    ax.set_xticks(x)
    ax.set_xticklabels([size_label(s) for s in sizes])
    ax.legend(loc="upper left", ncol=2, framealpha=0.9)
    ax.grid(axis="y", alpha=0.3)
    ax.set_ylim(bottom=0)

    path = os.path.join(outdir, "throughput_bars.png")
    fig.savefig(path)
    plt.close(fig)
    print(f"  Saved: {path}")


def plot_throughput_lines(data, schemes, sizes, outdir):
    """Line chart: throughput vs data size trends."""
    fig, ax = plt.subplots(figsize=(10, 6))

    for scheme in schemes:
        vals = [data[scheme][s]["throughput_mbps"] for s in sizes]
        ax.plot(range(len(sizes)), vals,
                color=SCHEME_COLORS.get(scheme, "#333333"),
                marker=SCHEME_MARKERS.get(scheme, "o"),
                label=SCHEME_LABELS.get(scheme, scheme),
                linewidth=1.5, markersize=6)

    ax.set_xlabel("Data Size")
    ax.set_ylabel("Throughput (MB/s)")
    ax.set_title("SM4 Throughput Scaling with Data Size")
    ax.set_xticks(range(len(sizes)))
    ax.set_xticklabels([size_label(s) for s in sizes])
    ax.legend(loc="best", ncol=2, framealpha=0.9)
    ax.grid(True, alpha=0.3)

    path = os.path.join(outdir, "throughput_lines.png")
    fig.savefig(path)
    plt.close(fig)
    print(f"  Saved: {path}")


# ============================================================================
# Speedup plots — ALL schemes on the SAME chart (including Zbb variants)
# ============================================================================

def plot_speedup_bars(data, schemes, sizes, outdir):
    """
    Grouped bar chart: speedup for ALL schemes (including default_loop=1.0,
    zbb_default, zbb_xbox) across all data sizes.
    """
    fig, ax = plt.subplots(figsize=(14, 6))
    x = np.arange(len(sizes))
    width = 0.11
    n = len(schemes)

    for i, scheme in enumerate(schemes):
        vals = [data[scheme][s]["speedup"] for s in sizes]
        offset = (i - (n - 1) / 2) * width
        bar_color = SCHEME_COLORS.get(scheme, "#333333")
        ax.bar(x + offset, vals, width,
               color=bar_color,
               label=SCHEME_LABELS.get(scheme, scheme),
               edgecolor="white", linewidth=0.3)

    # Baseline reference line
    ax.axhline(y=1.0, color="black", linestyle="--", linewidth=1.0,
               alpha=0.6, label="baseline (1.0×)")

    ax.set_xlabel("Data Size")
    ax.set_ylabel("Speedup (× vs default_loop baseline)")
    ax.set_title("SM4 Speedup Ratio — All Optimization Schemes (incl. Zbb ISA)")
    ax.set_xticks(x)
    ax.set_xticklabels([size_label(s) for s in sizes])
    ax.legend(loc="upper left", ncol=2, framealpha=0.9)
    ax.grid(axis="y", alpha=0.3)
    ax.set_ylim(bottom=0)

    path = os.path.join(outdir, "speedup_bars.png")
    fig.savefig(path)
    plt.close(fig)
    print(f"  Saved: {path}")


def plot_speedup_lines(data, schemes, sizes, outdir):
    """
    Line chart: speedup vs data size for ALL schemes
    (including default_loop=1.0, zbb_default, zbb_xbox).
    """
    fig, ax = plt.subplots(figsize=(12, 6))

    for scheme in schemes:
        vals = [data[scheme][s]["speedup"] for s in sizes]
        ax.plot(range(len(sizes)), vals,
                color=SCHEME_COLORS.get(scheme, "#333333"),
                marker=SCHEME_MARKERS.get(scheme, "o"),
                label=SCHEME_LABELS.get(scheme, scheme),
                linewidth=1.5, markersize=6)

    # Baseline reference
    ax.axhline(y=1.0, color="black", linestyle="--", linewidth=1.0,
               alpha=0.6)

    ax.set_xlabel("Data Size")
    ax.set_ylabel("Speedup (× vs default_loop baseline)")
    ax.set_title("SM4 Speedup Ratio vs Data Size — All Schemes")
    ax.set_xticks(range(len(sizes)))
    ax.set_xticklabels([size_label(s) for s in sizes])
    ax.legend(loc="best", ncol=2, framealpha=0.9)
    ax.grid(True, alpha=0.3)

    path = os.path.join(outdir, "speedup_lines.png")
    fig.savefig(path)
    plt.close(fig)
    print(f"  Saved: {path}")


def plot_speedup_avg(data, schemes, sizes, outdir):
    """
    Single bar chart: geometric mean speedup across all sizes.
    Includes ALL schemes (default_loop=1.0, zbb_default, zbb_xbox).
    """
    means = []
    labels = []
    colors = []
    for scheme in schemes:
        prod = 1.0
        for s in sizes:
            prod *= data[scheme][s]["speedup"]
        geo_mean = prod ** (1.0 / len(sizes))
        means.append(geo_mean)
        labels.append(SCHEME_LABELS.get(scheme, scheme))
        colors.append(SCHEME_COLORS.get(scheme, "#333333"))

    fig, ax = plt.subplots(figsize=(12, 5))
    bars = ax.bar(range(len(schemes)), means, color=colors, edgecolor="white")

    # Annotate each bar with the speedup value
    for bar, val in zip(bars, means):
        ax.text(bar.get_x() + bar.get_width() / 2,
                bar.get_height() + max(means) * 0.01,
                f"{val:.3f}×", ha="center", va="bottom",
                fontsize=10, fontweight="bold")

    # Baseline reference
    ax.axhline(y=1.0, color="black", linestyle="--", linewidth=1.0, alpha=0.6)

    ax.set_xlabel("Optimization Scheme")
    ax.set_ylabel("Geometric Mean Speedup (× vs baseline)")
    ax.set_title("Average Speedup Across All Data Sizes — All Schemes")
    ax.set_xticks(range(len(schemes)))
    ax.set_xticklabels(labels)
    ax.grid(axis="y", alpha=0.3)
    ax.set_ylim(bottom=0, top=max(means) * 1.15)

    path = os.path.join(outdir, "speedup_avg.png")
    fig.savefig(path)
    plt.close(fig)
    print(f"  Saved: {path}")


# ============================================================================
# Elapsed time plots
# ============================================================================

def plot_elapsed_loglog(data, schemes, sizes, outdir):
    """Log-log line chart: elapsed time vs data size for all schemes."""
    fig, ax = plt.subplots(figsize=(10, 6))

    for scheme in schemes:
        vals = [data[scheme][s]["elapsed_us"] for s in sizes]
        ax.loglog(sizes, vals,
                  color=SCHEME_COLORS.get(scheme, "#333333"),
                  marker=SCHEME_MARKERS.get(scheme, "o"),
                  label=SCHEME_LABELS.get(scheme, scheme),
                  linewidth=1.5, markersize=5)

    ax.set_xlabel("Data Size (bytes)")
    ax.set_ylabel("Elapsed Time (μs)")
    ax.set_title("SM4 Encryption Time (Log-Log Scale)")
    ax.legend(loc="upper left", ncol=2, framealpha=0.9)
    ax.grid(True, alpha=0.3, which="both")

    path = os.path.join(outdir, "elapsed_loglog.png")
    fig.savefig(path)
    plt.close(fig)
    print(f"  Saved: {path}")


def plot_elapsed_largest(data, schemes, sizes, outdir):
    """Horizontal bar chart: elapsed time at the largest data size."""
    largest = sizes[-1]
    pairs = [(scheme, data[scheme][largest]["elapsed_us"]) for scheme in schemes]
    pairs.sort(key=lambda x: x[1])  # fastest first

    s_names = [p[0] for p in pairs]
    s_vals = [p[1] / 1000.0 for p in pairs]  # μs → ms

    fig, ax = plt.subplots(figsize=(10, 5))
    colors = [SCHEME_COLORS.get(s, "#333333") for s in s_names]
    bars = ax.barh(range(len(s_names)), s_vals, color=colors, edgecolor="white")

    for bar, val in zip(bars, s_vals):
        ax.text(bar.get_width() + max(s_vals) * 0.01,
                bar.get_y() + bar.get_height() / 2,
                f"{val:.1f} ms", va="center", fontsize=9)

    ax.set_xlabel(f"Elapsed Time (ms) — {size_label(largest)} data")
    ax.set_ylabel("Scheme")
    ax.set_title(f"SM4 Encryption Time at {size_label(largest)} Data Size")
    ax.set_yticks(range(len(s_names)))
    ax.set_yticklabels([SCHEME_LABELS.get(s, s) for s in s_names])
    ax.invert_yaxis()
    ax.grid(axis="x", alpha=0.3)
    ax.set_xlim(right=max(s_vals) * 1.18)

    path = os.path.join(outdir, "elapsed_largest.png")
    fig.savefig(path)
    plt.close(fig)
    print(f"  Saved: {path}")


# ============================================================================
# ISA comparison
# ============================================================================

def plot_isa_comparison(data, schemes, sizes, outdir):
    """
    ISA-level comparison:
      (a) default_loop vs zbb_default — same algorithm, different ISA
      (b) xbox_merged vs zbb_xbox      — same algorithm, different ISA
      (c) ISA speedup ratio line chart
    """
    # (a) and (b): paired bar charts
    pairs = [
        ("default_loop", "zbb_default", "Default Loop: Baseline ISA vs Zbb ISA"),
        ("xbox_merged", "zbb_xbox", "4-Table XBox Merged: Baseline ISA vs Zbb ISA"),
    ]

    for s1, s2, title in pairs:
        if s1 not in data or s2 not in data:
            print(f"  Skip ISA comparison: {s1} or {s2} not in CSV data")
            continue

        fig, ax = plt.subplots(figsize=(10, 5))
        x = np.arange(len(sizes))
        width = 0.3

        v1 = [data[s1][s]["throughput_mbps"] for s in sizes]
        v2 = [data[s2][s]["throughput_mbps"] for s in sizes]

        ax.bar(x - width / 2, v1, width,
               color=SCHEME_COLORS[s1], label=SCHEME_LABELS.get(s1, s1),
               edgecolor="white")
        ax.bar(x + width / 2, v2, width,
               color=SCHEME_COLORS[s2], label=SCHEME_LABELS.get(s2, s2),
               edgecolor="white")

        ax.set_xlabel("Data Size")
        ax.set_ylabel("Throughput (MB/s)")
        ax.set_title(title)
        ax.set_xticks(x)
        ax.set_xticklabels([size_label(s) for s in sizes])
        ax.legend(framealpha=0.9)
        ax.grid(axis="y", alpha=0.3)

        fname = f"isa_comparison_{s1}_vs_{s2}.png"
        path = os.path.join(outdir, fname)
        fig.savefig(path)
        plt.close(fig)
        print(f"  Saved: {path}")

    # (c): ISA speedup ratio line chart
    isa_pairs = [
        ("default_loop", "zbb_default", "Default Loop (algorithm unchanged)"),
        ("xbox_merged", "zbb_xbox", "4-Table XBox Merged (algorithm unchanged)"),
    ]
    fig, ax = plt.subplots(figsize=(10, 5))
    has_any = False

    for s1, s2, label in isa_pairs:
        if s1 not in data or s2 not in data:
            continue
        has_any = True
        ratios = [data[s2][s]["throughput_mbps"] / data[s1][s]["throughput_mbps"]
                  for s in sizes]
        ax.plot(range(len(sizes)), ratios,
                marker="o", linewidth=1.8, markersize=7, label=label)

    if has_any:
        ax.axhline(y=1.0, color="black", linestyle="--", linewidth=1.0, alpha=0.6)
        ax.set_xlabel("Data Size")
        ax.set_ylabel("ISA Speedup Ratio (Zbb / Baseline ISA)")
        ax.set_title("Zbb ISA Extension: Pure ISA-Level Speedup")
        ax.set_xticks(range(len(sizes)))
        ax.set_xticklabels([size_label(s) for s in sizes])
        ax.legend(framealpha=0.9)
        ax.grid(True, alpha=0.3)

        path = os.path.join(outdir, "isa_speedup_ratio.png")
        fig.savefig(path)
        plt.close(fig)
        print(f"  Saved: {path}")
    else:
        plt.close(fig)
        print("  Skipped isa_speedup_ratio.png: no Zbb schemes in CSV")


# ============================================================================
# Optimization progression
# ============================================================================

def plot_optimization_progression(data, schemes, sizes, outdir):
    """
    Optimization path within each ISA group:
      (a) Baseline ISA: default_loop → unrolled → reduced_dep → xbox → xbox_merged
      (b) Zbb path:      default_loop → zbb_default → zbb_xbox (cross-ISA)
      (c) Cumulative speedup bars (Baseline ISA)
    """
    # (a) Within Baseline ISA
    baseline_path = ["default_loop", "unrolled", "reduced_dep", "xbox", "xbox_merged"]
    baseline_path = [s for s in baseline_path if s in data]

    if len(baseline_path) >= 2:
        fig, ax = plt.subplots(figsize=(12, 5))
        for scheme in baseline_path:
            vals = [data[scheme][s]["throughput_mbps"] for s in sizes]
            ax.plot(range(len(sizes)), vals,
                    color=SCHEME_COLORS.get(scheme, "#333333"),
                    marker=SCHEME_MARKERS.get(scheme, "o"),
                    label=SCHEME_LABELS.get(scheme, scheme),
                    linewidth=1.5, markersize=6)

        ax.set_xlabel("Data Size")
        ax.set_ylabel("Throughput (MB/s)")
        ax.set_title("Optimization Progression — Baseline ISA (rv64gc)")
        ax.set_xticks(range(len(sizes)))
        ax.set_xticklabels([size_label(s) for s in sizes])
        ax.legend(framealpha=0.9, ncol=2)
        ax.grid(True, alpha=0.3)

        path = os.path.join(outdir, "progression_baseline.png")
        fig.savefig(path)
        plt.close(fig)
        print(f"  Saved: {path}")

    # (b) Zbb path
    zbb_path = ["default_loop", "zbb_default", "zbb_xbox"]
    zbb_path = [s for s in zbb_path if s in data]

    if len(zbb_path) >= 2:
        fig, ax = plt.subplots(figsize=(10, 5))
        for scheme in zbb_path:
            vals = [data[scheme][s]["throughput_mbps"] for s in sizes]
            ax.plot(range(len(sizes)), vals,
                    color=SCHEME_COLORS.get(scheme, "#333333"),
                    marker=SCHEME_MARKERS.get(scheme, "o"),
                    label=SCHEME_LABELS.get(scheme, scheme),
                    linewidth=1.5, markersize=6)

        ax.set_xlabel("Data Size")
        ax.set_ylabel("Throughput (MB/s)")
        ax.set_title("Optimization Progression — Zbb ISA Path")
        ax.set_xticks(range(len(sizes)))
        ax.set_xticklabels([size_label(s) for s in sizes])
        ax.legend(framealpha=0.9)
        ax.grid(True, alpha=0.3)

        path = os.path.join(outdir, "progression_zbb.png")
        fig.savefig(path)
        plt.close(fig)
        print(f"  Saved: {path}")

    # (c) Cumulative speedup at largest data size — Baseline progression
    if len(baseline_path) >= 2:
        largest = sizes[-1]
        # Show only non-baseline steps for cumulative gain
        steps = [s for s in baseline_path if s != "default_loop"]
        if steps:
            sp = [data[s][largest]["speedup"] for s in steps]
            step_labels = [SCHEME_LABELS.get(s, s) for s in steps]
            step_colors = [SCHEME_COLORS.get(s, "#333333") for s in steps]

            fig, ax = plt.subplots(figsize=(9, 5))
            bars = ax.bar(range(len(sp)), sp, color=step_colors, edgecolor="white")
            for bar, val in zip(bars, sp):
                ax.text(bar.get_x() + bar.get_width() / 2,
                        bar.get_height() + 0.01,
                        f"{val:.3f}×", ha="center", va="bottom", fontweight="bold")

            ax.axhline(y=1.0, color="black", linestyle="--", linewidth=1.0, alpha=0.6)
            ax.set_xlabel("Optimization Step")
            ax.set_ylabel(f"Speedup at {size_label(largest)} (× vs baseline)")
            ax.set_title(f"Cumulative Speedup — Baseline ISA at {size_label(largest)}")
            ax.set_xticks(range(len(sp)))
            ax.set_xticklabels(step_labels)
            ax.grid(axis="y", alpha=0.3)
            ax.set_ylim(bottom=0, top=max(sp) * 1.15)

            path = os.path.join(outdir, "progression_baseline_cumulative.png")
            fig.savefig(path)
            plt.close(fig)
            print(f"  Saved: {path}")


# ============================================================================
# Main
# ============================================================================
def main():
    parser = argparse.ArgumentParser(
        description="SM4 Performance Analysis — Generate comparison plots from benchmark CSV"
    )
    parser.add_argument(
        "csv", nargs="?", default="sm4_results.csv",
        help="Path to sm4_results.csv (default: ./sm4_results.csv)"
    )
    parser.add_argument(
        "--outdir", "-o", default="figures",
        help="Output directory for figures (default: ./figures/)"
    )
    args = parser.parse_args()

    if not os.path.exists(args.csv):
        print(f"ERROR: CSV file not found: {args.csv}")
        print(f"       Run sm4_benchmark first to generate {args.csv}")
        sys.exit(1)

    ensure_dir(args.outdir)

    print(f"Loading data from: {args.csv}")
    data, schemes, sizes = load_data(args.csv)

    print(f"  Schemes found ({len(schemes)}):")
    for s in schemes:
        print(f"    - {s}")
    print(f"  Data sizes ({len(sizes)}): {[size_label(s) for s in sizes]}")
    print(f"\nGenerating plots → {os.path.abspath(args.outdir)}/\n")

    # ==================== Group 1: Throughput ====================
    print("=== Group 1: Throughput ===")
    plot_throughput_bars(data, schemes, sizes, args.outdir)
    plot_throughput_lines(data, schemes, sizes, args.outdir)

    # ==================== Group 2: Speedup (ALL schemes) ====================
    print("\n=== Group 2: Speedup (All Schemes incl. Zbb) ===")
    plot_speedup_bars(data, schemes, sizes, args.outdir)
    plot_speedup_lines(data, schemes, sizes, args.outdir)
    plot_speedup_avg(data, schemes, sizes, args.outdir)

    # ==================== Group 3: Elapsed Time ====================
    print("\n=== Group 3: Elapsed Time ===")
    plot_elapsed_loglog(data, schemes, sizes, args.outdir)
    plot_elapsed_largest(data, schemes, sizes, args.outdir)

    # ==================== Group 4: ISA Comparison ====================
    print("\n=== Group 4: ISA Comparison (Baseline vs Zbb) ===")
    plot_isa_comparison(data, schemes, sizes, args.outdir)

    # ==================== Group 5: Optimization Progression ====================
    print("\n=== Group 5: Optimization Progression ===")
    plot_optimization_progression(data, schemes, sizes, args.outdir)

    # ==================== Done ====================
    generated = os.listdir(args.outdir)
    print(f"\n{'='*60}")
    print(f"All plots saved to: {os.path.abspath(args.outdir)}/")
    print(f"Total figures: {len(generated)}")
    for fname in sorted(generated):
        print(f"  {fname}")
    print(f"Done.")


if __name__ == "__main__":
    main()
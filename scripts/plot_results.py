#!/usr/bin/env python3
"""
plot_results.py
Reads a comparison.csv produced by nvme_wt_sim and renders bar charts for
the key metrics side by side (baseline vs optimized). Not part of the C++
deliverable itself -- just a convenience for turning the raw numbers into
figures for a report/presentation.

Usage:
    python3 plot_results.py results/comparison.csv results/chart.png
"""
import csv
import sys

import matplotlib.pyplot as plt

def load(path):
    rows = {}
    with open(path) as f:
        reader = csv.reader(f)
        next(reader)  # header
        for metric, base, opt in reader:
            rows[metric] = (float(base), float(opt))
    return rows

def main():
    if len(sys.argv) < 2:
        print("usage: plot_results.py <comparison.csv> [output.png]")
        sys.exit(1)
    csv_path = sys.argv[1]
    out_path = sys.argv[2] if len(sys.argv) > 2 else "results/comparison_chart.png"

    data = load(csv_path)

    panels = [
        ("avg_latency_us", "Avg latency (us, lower is better)"),
        ("p99_latency_us", "p99 latency (us, lower is better)"),
        ("app_iops", "Application IOPS (higher is better)"),
        ("app_throughput_MBps", "Application throughput MB/s (higher is better)"),
        ("physical_ops", "Physical storage ops (lower is better)"),
        ("avg_queue_utilization_pct", "Avg queue utilization % (higher is better)"),
    ]

    fig, axes = plt.subplots(2, 3, figsize=(15, 8))
    axes = axes.flatten()

    for ax, (key, title) in zip(axes, panels):
        if key not in data:
            ax.axis("off")
            continue
        base, opt = data[key]
        bars = ax.bar(["Baseline", "Optimized"], [base, opt],
                       color=["#c0392b", "#2980b9"])
        ax.set_title(title, fontsize=10)
        ax.bar_label(bars, fmt="%.1f", fontsize=8)

    fig.suptitle("Write-Through Cache: Baseline vs Optimized NVMe Path", fontsize=13)
    fig.tight_layout(rect=[0, 0, 1, 0.95])
    fig.savefig(out_path, dpi=150)
    print(f"wrote {out_path}")

if __name__ == "__main__":
    main()

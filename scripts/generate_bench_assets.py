#!/usr/bin/env python3
"""Generate benchmark artifacts: CSV, Markdown tables, JSON bundle, and charts (PNG/SVG).

Environment / args:
    BENCH_EXPORT_DIR (optional): override output directory (default bench_artifacts)

Outputs (in export dir):
    fire_results.csv / .md
    population_results.csv / .md
    benchmarks.json (combined structured output)
    fire_speedup.(png|svg)
    fire_efficiency.(png|svg)
    population_point_range.(png|svg)   # point & range query comparison

Requires: matplotlib, pandas (install via pip if missing)
"""
from __future__ import annotations
import csv
import math
import os
import json
from dataclasses import dataclass
from typing import List

try:
    import pandas as pd  # type: ignore
    import matplotlib.pyplot as plt  # type: ignore
except ImportError:
    raise SystemExit("Please install dependencies: pip install pandas matplotlib")

ARTIFACT_DIR = os.environ.get("BENCH_EXPORT_DIR", "bench_artifacts")

os.makedirs(ARTIFACT_DIR, exist_ok=True)

@dataclass
class FireResult:
    model: str
    threads: int
    time_s: float
    speedup: float
    efficiency: float  # 0..1
    files_per_sec: float

FIRE_DATA: List[FireResult] = [
    FireResult("Row", 1, 2.079, 1.00, 1.00, 248.2),
    FireResult("Row", 2, 1.328, 1.57, 1.57/2, 388.4),
    FireResult("Row", 3, 1.006, 2.07, 2.07/3, 513.2),
    FireResult("Row", 4, 0.828, 2.51, 2.51/4, 622.9),
    FireResult("Row", 8, 0.806, 2.58, 2.58/8, 640.5),
    FireResult("Column", 1, 2.094, 1.00, 1.00, 246.4),
    FireResult("Column", 2, 1.340, 1.56, 1.56/2, 385.0),
    FireResult("Column", 3, 1.037, 2.02, 2.02/3, 497.4),
    FireResult("Column", 4, 0.874, 2.40, 2.40/4, 590.6),
    FireResult("Column", 8, 0.850, 2.46, 2.46/8, 606.8),
]

# Population results (with parallel variants). Times are microseconds (µs).
# Data from benchmark table (8-thread parallel where provided).
POPULATION_ROWS = [
    {"operation": "Sum", "row_serial_us": 1.992, "row_parallel_us": 163.750, "column_serial_us": 0.534, "column_parallel_us": 36.592},
    {"operation": "Average", "row_serial_us": 0.925, "row_parallel_us": 32.592, "column_serial_us": 0.400, "column_parallel_us": 37.267},
    {"operation": "Max", "row_serial_us": 0.900, "row_parallel_us": 23.267, "column_serial_us": 0.408, "column_parallel_us": 16.825},
    {"operation": "Min", "row_serial_us": 0.917, "row_parallel_us": 15.167, "column_serial_us": 0.442, "column_parallel_us": 16.333},
    {"operation": "Top-10", "row_serial_us": 11.175, "row_parallel_us": 29.684, "column_serial_us": 9.825, "column_parallel_us": 31.675},
    {"operation": "Point Query", "row_serial_us": 28.775, "row_parallel_us": 46.159, "column_serial_us": 0.133, "column_parallel_us": 0.200},
    {"operation": "Range (11y)", "row_serial_us": 37.558, "row_parallel_us": 22.092, "column_serial_us": 0.592, "column_parallel_us": 0.308},
]

def _add_derived_population_metrics(rows):
    for r in rows:
        r["column_advantage_serial"] = (r["row_serial_us"] / r["column_serial_us"]) if r["column_serial_us"] else None
        # Parallel advantage (avoid div-by-zero)
        if r["column_parallel_us"]:
            r["column_advantage_parallel"] = (r["row_parallel_us"] / r["column_parallel_us"]) if r["row_parallel_us"] else None
        else:
            r["column_advantage_parallel"] = None

_add_derived_population_metrics(POPULATION_ROWS)

# --- CSV export ---
fire_csv_path = os.path.join(ARTIFACT_DIR, "fire_results.csv")
with open(fire_csv_path, "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["model", "threads", "time_s", "speedup", "efficiency", "files_per_sec"])
    for r in FIRE_DATA:
        w.writerow([r.model, r.threads, f"{r.time_s:.3f}", f"{r.speedup:.2f}", f"{r.efficiency:.4f}", f"{r.files_per_sec:.1f}"])

pop_csv_path = os.path.join(ARTIFACT_DIR, "population_results.csv")
with open(pop_csv_path, "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["operation", "row_serial_us", "row_parallel_us", "column_serial_us", "column_parallel_us", "column_advantage_serial", "column_advantage_parallel"])
    for row in POPULATION_ROWS:
        w.writerow([
            row["operation"],
            f"{row['row_serial_us']:.3f}",
            f"{row['row_parallel_us']:.3f}",
            f"{row['column_serial_us']:.3f}",
            f"{row['column_parallel_us']:.3f}",
            f"{row['column_advantage_serial']:.2f}x",
            f"{row['column_advantage_parallel']:.2f}x" if row['column_advantage_parallel'] is not None else "-",
        ])

# --- Markdown export ---
fire_md_path = os.path.join(ARTIFACT_DIR, "fire_results.md")
with open(fire_md_path, "w") as f:
    f.write("| Model | Threads | Time (s) | Speedup | Efficiency | Files/sec |\n")
    f.write("|-------|---------|----------|---------|------------|-----------|\n")
    for r in FIRE_DATA:
        f.write(f"| {r.model} | {r.threads} | {r.time_s:.3f} | {r.speedup:.2f}x | {r.efficiency*100:.1f}% | {r.files_per_sec:.1f} |\n")

pop_md_path = os.path.join(ARTIFACT_DIR, "population_results.md")
with open(pop_md_path, "w") as f:
    f.write("| Operation | Row Serial (µs) | Row Parallel (µs) | Column Serial (µs) | Column Parallel (µs) | Col Adv Serial | Col Adv Parallel |\n")
    f.write("|-----------|----------------:|------------------:|------------------:|--------------------:|---------------:|-----------------:|\n")
    for row in POPULATION_ROWS:
        f.write(
            f"| {row['operation']} | {row['row_serial_us']:.3f} | {row['row_parallel_us']:.3f} | {row['column_serial_us']:.3f} | {row['column_parallel_us']:.3f} | {row['column_advantage_serial']:.2f}x | {row['column_advantage_parallel']:.2f}x |\n"
        )

# --- JSON bundle ---
json_path = os.path.join(ARTIFACT_DIR, "benchmarks.json")
with open(json_path, "w") as jf:
    json.dump({
        "fire": [r.__dict__ for r in FIRE_DATA],
        "population": POPULATION_ROWS,
        "metadata": {
            "fire_dataset": {"files": 516, "measurements": 1167525, "sites": 1398},
            "population_dataset": {"countries": 266, "years": 65},
            "generated_with": "generate_bench_assets.py"
        }
    }, jf, indent=2)

# --- Charts ---
# Fire speedup bar chart
import itertools

row_threads = [r.threads for r in FIRE_DATA if r.model == "Row"]
row_speedup = [r.speedup for r in FIRE_DATA if r.model == "Row"]
col_speedup = [r.speedup for r in FIRE_DATA if r.model == "Column"]

fig, ax = plt.subplots(figsize=(6,4))
ax.plot(row_threads, row_speedup, marker='o', label='Row')
ax.plot(row_threads, col_speedup, marker='s', label='Column')
ax.set_xlabel('Threads')
ax.set_ylabel('Speedup (vs 1 thread)')
ax.set_title('Fire Data Speedup')
ax.grid(alpha=0.25)
ax.legend()
fig.tight_layout()
fig.savefig(os.path.join(ARTIFACT_DIR, 'fire_speedup.png'), dpi=160)
fig.savefig(os.path.join(ARTIFACT_DIR, 'fire_speedup.svg'))
plt.close(fig)

# Fire efficiency chart
row_eff = [r.efficiency*100 for r in FIRE_DATA if r.model == "Row"]
col_eff = [r.efficiency*100 for r in FIRE_DATA if r.model == "Column"]
fig, ax = plt.subplots(figsize=(6,4))
ax.plot(row_threads, row_eff, marker='o', label='Row')
ax.plot(row_threads, col_eff, marker='s', label='Column')
ax.set_xlabel('Threads')
ax.set_ylabel('Efficiency (%)')
ax.set_title('Fire Data Parallel Efficiency')
ax.grid(alpha=0.25)
ax.legend()
fig.tight_layout()
fig.savefig(os.path.join(ARTIFACT_DIR, 'fire_efficiency.png'), dpi=160)
fig.savefig(os.path.join(ARTIFACT_DIR, 'fire_efficiency.svg'))
plt.close(fig)

# Population point vs range query (serial) chart
point = next(r for r in POPULATION_ROWS if r['operation'] == 'Point Query')
range_op = next(r for r in POPULATION_ROWS if r['operation'].startswith('Range'))
fig, ax = plt.subplots(figsize=(6,4))
labels = ['Point Serial', 'Point Parallel', 'Range Serial', 'Range Parallel']
values = [point['column_serial_us'], point['column_parallel_us'], range_op['column_serial_us'], range_op['column_parallel_us']]
ax.bar(labels, values, color=['#4c72b0','#55a868','#4c72b0','#55a868'])
ax.set_ylabel('Time (µs)')
ax.set_title('Population Analytical Latency (Column Model)')
for i,v in enumerate(values):
    ax.text(i, v*1.02, f"{v:.3f}", ha='center', fontsize=8)
fig.tight_layout()
fig.savefig(os.path.join(ARTIFACT_DIR, 'population_point_range.png'), dpi=160)
fig.savefig(os.path.join(ARTIFACT_DIR, 'population_point_range.svg'))
plt.close(fig)

print("Artifacts written to", ARTIFACT_DIR)

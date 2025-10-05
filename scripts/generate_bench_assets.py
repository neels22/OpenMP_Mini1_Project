#!/usr/bin/env python3
"""Generate benchmark CSV, Markdown tables, and simple charts (PNG/SVG) for README usage.

Outputs (written to bench_artifacts/):
  fire_results.csv
  population_results.csv
  fire_results.md
  population_results.md
  fire_speedup.png / fire_speedup.svg
  fire_efficiency.png / fire_efficiency.svg

Requires: matplotlib, pandas (install via pip if missing)
"""
from __future__ import annotations
import csv
import math
import os
from dataclasses import dataclass
from typing import List

try:
    import pandas as pd  # type: ignore
    import matplotlib.pyplot as plt  # type: ignore
except ImportError:
    raise SystemExit("Please install dependencies: pip install pandas matplotlib")

ARTIFACT_DIR = "bench_artifacts"

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

# Population results (serial only for comparison focus)
POPULATION_ROWS = [
    {"operation": "Sum", "row_serial_us": 1.992, "column_serial_us": 0.534, "speedup": 1.992/0.534},
    {"operation": "Point Query", "row_serial_us": 28.775, "column_serial_us": 0.133, "speedup": 28.775/0.133},
    {"operation": "Range (11y)", "row_serial_us": 37.558, "column_serial_us": 0.592, "speedup": 37.558/0.592},
]

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
    w.writerow(["operation", "row_serial_us", "column_serial_us", "column_advantage"])
    for row in POPULATION_ROWS:
        w.writerow([
            row["operation"],
            f"{row['row_serial_us']:.3f}",
            f"{row['column_serial_us']:.3f}",
            f"{row['speedup']:.2f}x",
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
    f.write("| Operation | Row Serial (µs) | Column Serial (µs) | Column Advantage |\n")
    f.write("|-----------|----------------:|-------------------:|-----------------:|\n")
    for row in POPULATION_ROWS:
        f.write(
            f"| {row['operation']} | {row['row_serial_us']:.3f} | {row['column_serial_us']:.3f} | {row['speedup']:.2f}x |\n"
        )

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

# Efficiency chart
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

print("Artifacts written to", ARTIFACT_DIR)

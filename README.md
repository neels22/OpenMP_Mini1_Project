## OpenMP Fire Data Processing & Population Analytics

High-performance C++17 + OpenMP framework comparing two data layouts for environmental monitoring and demographic analytics:
- Fire data: row-oriented (site grouped) vs column-oriented (field grouped)
- Population data: row model vs column model for aggregation and lookup patterns

Key result: Row-oriented fire model reaches 640.5 files/sec with 2.58x speedup (79–89% efficiency). Column model dominates analytical (population) point/range queries (up to 200x faster).

---
### 1. Quick Start
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel

# Fire data benchmark (both models)
./OpenMP_Mini1_Project_app --fire --threads 8 --repetitions 3

# Population analytics benchmark
./OpenMP_Mini1_Project_app --threads 8 --repetitions 5

# Individual fire model tests
./OpenMP_Mini1_Project_fire_test
./OpenMP_Mini1_Project_fire_column_test
```

Dependencies: CMake ≥3.16, C++17 compiler, OpenMP (macOS: `brew install cmake libomp`).

---
### 2. Fire Data Performance (516 CSV files, 1,167,525 measurements, 1,398 sites)
| Model  | Threads | Time (s) | Speedup | Efficiency | Files/sec |
|--------|---------|----------|---------|------------|-----------|
| Row    | 1 | 2.079 | 1.00x | -    | 248.2 |
| Row    | 4 | 0.828 | 2.51x | 81%  | 622.9 |
| Row    | 8 | 0.806 | 2.58x | 79%  | 640.5 |
| Column | 1 | 2.094 | 1.00x | -    | 246.4 |
| Column | 4 | 0.874 | 2.40x | 25%  | 590.6 |
| Column | 8 | 0.850 | 2.46x | 12%  | 606.8 |

Summary: Row model slightly faster baseline (+1%), better scaling (+0.12 absolute speedup), 6× higher efficiency at 8 threads.

---
### 3. Population Analytics (266 countries × 65 years)
| Operation    | Row Serial | Column Serial | Column Advantage |
|--------------|-----------:|--------------:|-----------------:|
| Sum          | 1.99 μs | 0.53 μs | 3.7x |
| Point Query  | 28.78 μs | 0.13 μs | 216x |
| Range (11y)  | 37.56 μs | 0.59 μs | 63.5x |

Summary: Column layout wins analytic access (contiguous per-field arrays enable cache streaming + O(1) indexing).

---
### 4. Model Overview
| Aspect | Fire Row Model | Fire Column Model |
|--------|----------------|-------------------|
| Layout | Per-site hierarchical containers | One vector per field (13 columns) |
| Strengths | Parallel ingestion, site queries, geographic grouping | Aggregations, parameter/time series scans |
| Weakness | Field-specific scans need iteration across sites | Poor parallel efficiency merging/thread overhead |
| Best Use | Real-time ingestion, site analytics | Scientific/statistical post-processing |

---
### 5. Parallel Strategy (Fire)
Dynamic file-level work distribution + thread‑local staging → merge at end.
```cpp
#pragma omp parallel num_threads(n)
{
    ThreadLocal tl;
    #pragma omp for schedule(dynamic,1)
    for (size_t i=0;i<files.size();++i) tl.process(files[i]);
    // single/critical: merge tl into global
}
```

---
### 6. Architecture Selection
| Scenario | Recommended | Reason |
|----------|-------------|--------|
| High-throughput CSV ingestion | Fire Row | Better scaling & efficiency |
| Site / geographic queries | Fire Row | Native hierarchy |
| Bulk statistical analysis | Fire Column | Column locality |
| Point/time-range lookups (population) | Column | Direct indexed access |
| Mixed ingestion + queries | Hybrid | Row for load, column for analytics |

---
### 7. CLI Flags
| Flag | Description | Default |
|------|-------------|---------|
| `--fire, -f` | Run fire dual-model benchmark | off |
| `--threads N, -t N` | OpenMP threads | 4 |
| `--repetitions N, -r N` | Benchmark repetitions (avg) | 5 |
| `--help, -h` | Show usage | - |

---
### 8. Project Structure (abridged)
```
interface/      # Model & service interfaces
src/            # Implementations & benchmarks
  fireRowModel.cpp
  fireColumnModel.cpp
  populationModel*.cpp
  main.cpp
tests/          # Validation & equivalence tests
data/FireData/  # (CSV files not included)
```

---
### 9. Key Insights
1. Layout choice drives both cache behavior and parallel efficiency; they optimize different phases.
2. Row model: best end-to-end ingestion + acceptable analytics (use when latency matters).
3. Column model: unrivaled for fine-grained analytical queries (after data loaded/cleaned).
4. Optimal thread count: 4 gives near-peak speed with slightly higher efficiency than 8.

---
### 10. Minimal Usage Pattern
```cpp
FireRowModel fire;
fire.readFromDirectoryParallel("data/FireData", 4);
std::cout << fire.siteCount() << " sites\n";

FireColumnModel fireCol;
fireCol.readFromDirectory("data/FireData", 1); // serial ingestion
auto pm = fireCol.getIndicesByParameter("PM2.5");
```

---
### 11. License
MIT License

Performance validated: 640.5 files/sec (row), 2.58x speedup, consistent analytical correctness across models.

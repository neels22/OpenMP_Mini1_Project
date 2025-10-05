# Population Analytics Benchmark (Row vs Column) – C++17 + OpenMP

A focused benchmark and reference implementation comparing two in‑memory data layouts for population analytics. Both the row‑oriented (country-major) and column‑oriented (year-major) models implement a shared analytics interface, enabling direct performance attribution to memory layout rather than code divergence.

---
## 1. Objectives
| Goal | Description |
|------|-------------|
| Layout Impact | Measure how row vs column layout affects aggregation, ranking, and point queries |
| Parallel Scaling | Evaluate OpenMP overhead/benefit per operation category |
| Clean Architecture | Provide interface-based design with zero duplicated analytics logic |
| Deterministic Benchmarks | Fixed repetitions with mean timing + correctness cross-checks |

---
## 2. Architecture Overview
```
interface/
  populationModel.hpp              # Row model (vector<PopulationRow>)
  populationModelColumn.hpp        # Column model (vector<year-columns>)
  population_service_interface.hpp # IPopulationService abstraction
  service.hpp                      # Concrete services (row & column)
  benchmark_runner.hpp / benchmark_utils.hpp
  utils.hpp / constants.hpp / readcsv.hpp
src/
  populationModel.cpp              # Row ingestion + indexing
  populationModelColumn.cpp        # Column ingestion + indexing
  service.cpp / service_column.cpp # Serial + OpenMP analytics implementations
  benchmark_runner.cpp             # Generic templated orchestration
  benchmark_utils.cpp              # CLI, validation, timing utilities
  synthetic_row_benchmark.cpp      # Synthetic CSV generator harness
  main.cpp                         # Entry point: loads data, runs full suite
tests/
  basic_tests.cpp                  # Utility + equivalence tests
```

### Key Abstractions
| Layer | Responsibility |
|-------|----------------|
| Models | Physical storage & raw indexed access (row vs column) |
| Services | Analytics methods (sum, avg, min, max, top-N, point, range) via `IPopulationService` |
| Benchmark Runner | Polymorphic execution over any service implementation |
| Utilities | Timing, statistics, parsing, validation, synthetic data |

---
## 3. Supported Analytics API
| Category | Method | Notes |
|----------|--------|------|
| Aggregation | sumPopulationForYear | OpenMP reduction in parallel path |
| Aggregation | averagePopulationForYear | Mean of available entries |
| Aggregation | maxPopulationForYear | Parallel max reduction |
| Aggregation | minPopulationForYear | Parallel min reduction |
| Ranking | topNCountriesByPopulationInYear | Per-thread min-heaps merged |
| Point Query | populationForCountryInYear | Column: direct indexing; Row: map + row vector |
| Time Series | populationOverYearsForCountry | Row contiguous; Column reassembled |

---
## 4. Build & Run
Prerequisites: C++17 compiler, CMake ≥ 3.16, OpenMP (macOS: `brew install libomp`).

```bash
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Real dataset benchmark
./build/OpenMP_Mini1_Project_app -r 5 -t 8

# Synthetic CSV generation + benchmark rerun
./build/OpenMP_Mini1_Project_row_benchmark

# Tests
./build/OpenMP_Mini1_Project_tests
```
Custom dataset path:
```bash
CSV_PATH=data/PopulationData/population_synthetic.csv ./build/OpenMP_Mini1_Project_app -r 5 -t 8
```
CLI flags:
```
  -r, --reps N     Repetitions (mean reported)
  -t, --threads N  Thread count for parallel variants
  -h, --help       Usage
```

---
## 5. Benchmark Results (Fresh Runs)
All times are mean microseconds (µs). Parallel uses 8 threads. Results validated: row vs column and serial vs parallel produce identical numeric outputs for each operation.

### 5.1 Real Dataset (266 countries × 65 years, reps=5)
| Operation | Row Serial | Row Parallel | Column Serial | Column Parallel | Column vs Row Serial Speedup |
|-----------|------------|--------------|---------------|-----------------|------------------------------|
| Sum | 1.992 | 163.750 | 0.534 | 36.592 | 3.73x |
| Average | 0.925 | 32.592 | 0.400 | 37.267 | 2.31x |
| Max | 0.900 | 23.267 | 0.408 | 16.825 | 2.21x |
| Min | 0.917 | 15.167 | 0.442 | 16.333 | 2.07x |
| Top-10 | 11.175 | 29.684 | 9.825 | 31.675 | 1.14x |
| Point Lookup | 28.775 | 46.159 | 0.133 | 0.200 | 216x |
| Range (11 yrs) | 37.558 | 22.092 | 0.592 | 0.308 | 63.5x |

### 5.2 Synthetic Generation Run (Generator wrote 200k × 100 CSV; app currently reloads curated dataset afterward)
| Operation | Row Serial | Row Parallel | Column Serial | Column Parallel | Column vs Row Serial Speedup |
|-----------|------------|--------------|---------------|-----------------|------------------------------|
| Sum | 1.266 | 115.948 | 0.422 | 59.614 | 3.00x |
| Average | 1.078 | 65.224 | 0.490 | 61.620 | 2.20x |
| Max | 0.813 | 33.068 | 0.469 | 41.666 | 1.73x |
| Min | 0.880 | 33.667 | 0.380 | 29.959 | 2.32x |
| Top-10 | 8.542 | 45.896 | 6.896 | 45.385 | 1.24x |
| Point Lookup | 21.604 | 19.156 | 0.094 | 0.036 | 230x |
| Range (11 yrs) | 25.636 | 20.724 | 0.406 | 0.302 | 63.2x |

> NOTE: A future enhancement will re-load the generated large synthetic CSV into in-memory models (instead of reverting to the small curated dataset) to surface true large-scale timings inside the same run.

---
## 6. Analysis & Insights
1. Column layout is consistently 2–4× faster on per-year aggregations due to contiguous memory streaming and reduced cache miss rate.
2. Point and short range queries show 100×–200×+ gains for the column model (direct index arithmetic vs hash + indirect row access).
3. Parallel overhead dominates at current dataset size; reductions & top-N only justify threading when country count grows substantially.
4. Row layout best fits workloads performing repeated full time series extraction per country.
5. Per-thread min-heap + merge reduces sort cost for top-N; benefits scale with larger N or bigger row counts.

---
## 7. Design Techniques
| Concern | Solution | Effect |
|---------|----------|--------|
| Code Duplication | `IPopulationService` interface | Single polymorphic benchmark path |
| Aggregation Parallelism | OpenMP reductions | Simple + race-free |
| Ranking Efficiency | Thread-local min-heaps | Limited contention & memory traffic |
| Validation | Serial vs parallel & row vs column cross-check | Guarantees semantic parity |
| Safety | Defensive bounds & lookups return 0 | No undefined behavior on bad input |

---
## 8. Layout Selection Guide
| Scenario | Choose | Reason |
|----------|-------|-------|
| Global per-year scans | Column | Contiguous column arrays |
| Frequent (country,year) lookups | Column | O(1) index math |
| Long per-country time series | Row | Data already contiguous |
| Mixed analytics + ranking | Column | Faster scans dominate |
| Very small dataset + simplicity | Row | Overhead differences negligible |

---
## 9. Testing
```bash
./build/OpenMP_Mini1_Project_tests
```
Coverage:
- Utility functions (timing, parsing, statistics)
- Command-line configuration & validation
- Row vs column equivalence for every operation
- Serial vs parallel correctness (fail-fast on mismatch)

---
## 10. Roadmap
Short Term:
- In-process benchmarking of true large synthetic dataset
- Thread scaling matrix (1..N) auto-export (Markdown/CSV)

Mid Term:
- SIMD/vectorized column reductions
- Compressed storage experiments (delta, RLE, dictionary)
- Additional analytics: growth %, rolling averages, percentile, median

Long Term:
- Execution backend abstraction (OpenMP / std::execution / TBB)
- Streaming ingestion + incremental rolling aggregates

---
## 11. Quick Reference
```bash
# Build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel
# Real dataset benchmark
./build/OpenMP_Mini1_Project_app -r 5 -t 8
# Synthetic generation + benchmark
./build/OpenMP_Mini1_Project_row_benchmark
# Tests
./build/OpenMP_Mini1_Project_tests
```

---
## 12. License
MIT

## 13. Acknowledgements
Created as a concise exploration of how memory layout *is* a performance primitive. Provides a clean, reproducible C++17 baseline for experimentation with locality, indexing, and parallel reductions.

*End of README*

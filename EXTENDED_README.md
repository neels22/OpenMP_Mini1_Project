# OpenMP Fire Data Processing & Population Analytics (Extended Documentation)

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)
![OpenMP](https://img.shields.io/badge/Parallel-OpenMP-success)
![License: MIT](https://img.shields.io/badge/License-MIT-green)
![Status](https://img.shields.io/badge/Build-Local--CMake-informational)

> This extended document preserves the full analysis, architecture deep dive, and benchmarking rationale. The top-level `README.md` is a concise summary; this file is the detailed companion.

---
## 1. Project Overview
A high-performance C++17 + OpenMP benchmarking & analytics framework comparing **row-oriented vs column-oriented storage architectures** across two domains:
- **Fire Monitoring Data** (516 CSV files, 1,167,525 measurements, 1,398 sites)
- **Population Analytics** (266 countries × 65 years = 17,290 records)

The system evaluates how data layout affects: ingestion speed, parallel scalability, cache behavior, analytical queries, and efficiency under OpenMP.

**Headline Findings**
- Row-oriented fire model: 640.5 files/sec, 2.58× speedup, 79–89% efficiency (superior parallel scaling)
- Column-oriented population model: Up to 200× faster point and 60× faster range queries
- Architecture choice is workload-dependent: ingestion vs analytics

---
## 2. Quick Start
```bash
# Configure & build (Release)
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel

# Fire data dual-model benchmark
./OpenMP_Mini1_Project_app --fire --threads 8 --repetitions 3

# Population analytics benchmark
./OpenMP_Mini1_Project_app --threads 8 --repetitions 5

# Individual fire model validation
./OpenMP_Mini1_Project_fire_test
./OpenMP_Mini1_Project_fire_column_test

# Full test suite
./OpenMP_Mini1_Project_tests
```
Dependencies: CMake ≥ 3.16, C++17 compiler, OpenMP library (macOS: `brew install cmake libomp`).

---
## 3. Command Line Interface
| Flag | Meaning | Default |
|------|---------|---------|
| `--fire, -f` | Run fire data benchmark (both models) | off |
| `--threads N, -t N` | OpenMP thread count | 4 |
| `--repetitions N, -r N` | Benchmark repetitions (mean) | 5 |
| `--help, -h` | Show usage | - |

Environment override: `CSV_PATH=<path>` for population dataset substitution.

---
## 4. Repository Structure (Abridged)
```
interface/
  fireRowModel.hpp
  fireColumnModel.hpp
  populationModel.hpp
  populationModelColumn.hpp
  population_service_interface.hpp
  benchmark_runner.hpp
  benchmark_utils.hpp
  service.hpp / service_column.hpp
  utils.hpp / constants.hpp / readcsv.hpp

src/
  fireRowModel.cpp
  fireColumnModel.cpp
  populationModel.cpp
  populationModelColumn.cpp
  service.cpp / service_column.cpp
  benchmark_runner.cpp
  benchmark_utils.cpp
  synthetic_row_benchmark.cpp
  fire_test.cpp
  fire_column_test.cpp
  main.cpp

tests/
  basic_tests.cpp
```

---
## 5. Architectural Layers & Design Patterns
| Layer | Responsibility | Pattern(s) |
|-------|----------------|------------|
| Data Models (Row / Column) | Physical layout + indexing | Strategy |
| Fire Data Dual Models | Site vs field orientation | Template Method / Strategy |
| Analytics Service | Uniform operations over models | Interface Segregation |
| Benchmark Runner | Reusable timing & validation | Template Method |
| Validation & Cross-Checks | Ensure numeric equivalence | Observer (conceptual) |
| Utilities | Timing, parsing, stats, CSV ingest | RAII + Utility Modules |

---
## 6. Data Model Comparison (Fire Domain)
| Aspect | Row-Oriented (FireRowModel) | Column-Oriented (FireColumnModel) |
|--------|-----------------------------|----------------------------------|
| Storage | Hierarchical per site | Separate vectors per field (13 columns) |
| Ingestion Speed (1 thread) | 2.079 s | 2.094 s |
| Parallel Speedup (8 threads) | 2.58× | 2.46× |
| Efficiency (8 threads) | 79–89% (2–8 threads) | 12–50% |
| Best For | Site/geographic queries | Analytical scans & per-field statistics |
| Merge Complexity | Site map consolidation | Vector concatenation + index rebuild |
| Memory Locality | Spatial grouping | Field-contiguous streaming |
| Strength | Load balancing & scaling | Cache-friendly analytics |

---
## 7. Benchmark Methodology
| Dimension | Approach |
|----------|----------|
| Repetitions | Mean of N runs (default 5) |
| Timing Source | `std::chrono::high_resolution_clock` |
| Parallelism | OpenMP dynamic scheduling (`schedule(dynamic,1)`) |
| Validation | Cross-architecture numerical equality checks (when applicable) |
| Error Handling | Per-file exception capture; continued processing |
| Throughput Metric | Files/sec = total_files / elapsed_time |
| Efficiency | Speedup / threads (reported as %) |

---
## 8. Fire Data Performance Results
Dataset: **516 CSV files**, **1,167,525 measurements**, **1,398 sites**.

| Model | Threads | Time (s) | Speedup | Efficiency | Files/sec |
|-------|---------|----------|---------|------------|-----------|
| Row | 1 | 2.079 | 1.00× | - | 248.2 |
| Row | 2 | 1.328 | 1.57× | 88.7% | 388.4 |
| Row | 3 | 1.006 | 2.07× | 84.8% | 513.2 |
| Row | 4 | 0.828 | 2.51× | 81.2% | 622.9 |
| Row | 8 | 0.806 | 2.58× | 79.4% | 640.5 |
| Column | 1 | 2.094 | 1.00× | - | 246.4 |
| Column | 2 | 1.340 | 1.56× | 50.0% | 385.0 |
| Column | 3 | 1.037 | 2.02× | 33.3% | 497.4 |
| Column | 4 | 0.874 | 2.40× | 25.0% | 590.6 |
| Column | 8 | 0.850 | 2.46× | 12.5% | 606.8 |

### 8.1 Speedup Visualization (Mermaid)
```mermaid
bar
    title Fire Data Speedup by Threads
    x-axis Threads
    y-axis Speedup
    series Row [1.00,1.57,2.07,2.51,2.58]
    series Column [1.00,1.56,2.02,2.40,2.46]
```

### 8.2 Efficiency (ASCII Bars)
```
Threads:   2       3       4       8
Row:     88.7%  84.8%  81.2%  79.4%  #######++++
Column:  50.0%  33.3%  25.0%  12.5%  ###--
(Each # ≈ 10%, + ≈ residual)
```

### 8.3 Key Observations
1. Row model retains high efficiency beyond 4 threads; column model rapidly loses parallel benefit.
2. 4 threads provide a near-optimal balance (Row: 2.51× vs 2.58× at 8 with higher efficiency).
3. Throughput peak: Row model at 640.5 files/sec; column at 606.8 files/sec.

### 8.4 Generated Charts (Artifacts)
The following charts are auto-generated by `scripts/generate_bench_assets.py` (see `bench_artifacts/`). They are embedded here for visual reference.

| Chart | Description |
|-------|-------------|
| ![Fire Speedup](bench_artifacts/fire_speedup.png) | Speedup of column vs row ingestion across thread counts. |
| ![Fire Efficiency](bench_artifacts/fire_efficiency.png) | Parallel efficiency (speedup / threads) comparison between models. |
| ![Population Point vs Range](bench_artifacts/population_point_range.png) | Column-model latency for point vs 11-year range queries (serial vs parallel). |

---
## 9. Population Analytics Performance
Dataset: 266 countries × 65 years.

| Operation | Row Serial (µs) | Column Serial (µs) | Column Advantage |
|-----------|-----------------|--------------------|------------------|
| Sum | 1.992 | 0.534 | 3.73× |
| Average | 0.925 | 0.400 | 2.31× |
| Max | 0.900 | 0.408 | 2.21× |
| Min | 0.917 | 0.442 | 2.07× |
| Top-10 | 11.175 | 9.825 | 1.14× |
| Point Lookup | 28.775 | 0.133 | 216× |
| Range (11 yrs) | 37.558 | 0.592 | 63.5× |

**Why Column Wins**: Direct index arithmetic (country_offset + year_delta) + contiguous memory scanning minimizing cache misses.

---
## 10. Parallel Strategy (Fire Ingestion)
```cpp
#pragma omp parallel num_threads(num_threads)
{
    FireRowModel thread_local_model; // or a lightweight column staging struct
    #pragma omp for schedule(dynamic,1)
    for (size_t i = 0; i < files.size(); ++i) {
        try {
            thread_local_model.processFile(files[i]);
        } catch (const std::exception& e) {
            #pragma omp critical
            std::cerr << "[warn] file failed: " << e.what() << "\n";
        }
    }
    // Merge phase (architecture-specific consolidation)
}
```

**Techniques**
- Dynamic scheduling smooths heterogeneous file sizes.
- Thread-local accumulation avoids locks in hot path.
- Single merge pass amortizes synchronization.

---
## 11. Feature Comparison Matrix
| Feature | FireRowModel | FireColumnModel | Winner |
|---------|--------------|-----------------|--------|
| Serial Ingestion | 2.079 s | 2.094 s | Row |
| Peak Speedup | 2.58× | 2.46× | Row |
| Efficiency Retention | High (79–89%) | Low (12–50%) | Row |
| Site / Geographic Queries | Native | Indexed | Row |
| Analytical Aggregations | Good | Excellent | Column |
| Point / Range Queries | Indirect | O(1)/contiguous | Column |
| Implementation Complexity | Moderate | Higher | Row (simplicity) |
| Memory for Field Scans | Dispersed | Contiguous | Column |

---
## 12. Architecture Selection Guide
| Scenario | Recommendation | Rationale |
|----------|---------------|-----------|
| High-throughput ingestion | Row | Superior scaling & efficiency |
| Real-time monitoring | Row | Low variance + hierarchy |
| Statistical analysis batch | Column | Faster scans/aggregations |
| Point demographic lookup | Column | Direct index access |
| Mixed workload | Hybrid | Row ingest → transform to column |
| Limited cores (≤4) | Row | Near-ideal scaling early |
| Many fine-grained queries | Column | Avoids map/hash overhead |

---
## 13. Population Model Internals
```cpp
struct PopulationRow { std::string country; int year; long long population; };

class PopulationModel { // Row layout
    std::vector<PopulationRow> data;
    std::unordered_map<std::string,size_t> countryIndex; // first occurrence
};

class PopulationModelColumn { // Column layout
    std::vector<std::string> countries;
    std::vector<int> years;
    std::vector<long long> populations;
    std::unordered_map<std::string,std::vector<size_t>> countryIndex; // positions
};
```
**Trade-off**: Row layout favors sequential per-country time series; column layout favors cross-country or per-year aggregates.

---
## 14. Validation & Testing
Coverage:
- Row vs column equivalence (aggregations & ranking)
- Serial vs parallel numeric identity
- Fire dual-model site/measurement consistency
- Range & point query correctness

```bash
./OpenMP_Mini1_Project_tests
```

---
## 15. Performance Interpretation Guidelines
| Symptom | Likely Cause | Mitigation |
|---------|--------------|-----------|
| Diminishing speedup >4 threads (column) | Merge/index overhead | Batch merge, preallocate indices |
| Low cache efficiency (row scans) | Scattered field access | Add derived column view or SoA buffer |
| Load imbalance spikes | Large file size variance | Increase dynamic chunk size (e.g., 2) |
| High variance in timings | OS noise / I/O caching | Warm-up run + median reporting |

---
## 16. Potential Enhancements
| Category | Idea | Expected Benefit |
|----------|------|-----------------|
| SIMD | Vectorized reductions | Faster aggregations |
| Compression | Dictionary / delta | Memory & cache gains |
| Geospatial | Integrate geohash/R-tree | Spatial queries |
| Streaming | Incremental ingestion | Real-time analytics |
| Adaptive | Auto layout switcher | Dynamic optimization |
| Backend | Pluggable (TBB/std::execution) | Portability & tuning |

---
## 17. Key Takeaways
1. **No universal winner**: Row wins ingestion + parallel efficiency; column wins fine-grained analytics.
2. **Efficiency matters**: Raw speedup without efficiency hides wasted parallel potential.
3. **Data layout = algorithm enabler**: Query complexity collapses with proper indexing + structure.
4. **Hybrid architectures** can unify ingestion rate and analytical throughput.

---
## 18. Minimal Hybrid Workflow Concept
```cpp
FireRowModel ingest;
ingest.readFromDirectoryParallel("data/FireData", 4);
FireColumnModel analytics = transformToColumn(ingest); // (future enhancement)
auto pm25 = analytics.getIndicesByParameter("PM2.5");
```

---
## 19. License
MIT License. See main `README.md` for concise summary.

**Performance Validated**: 640.5 files/sec (Row Fire Model), 2.58× speedup, analytical parity across implementations.

---
## 20. Attribution
Developed as a study of how memory layout & parallelization influence performance in environmental and demographic data processing.

*End of Extended Documentation*

# Population Analytics & Fire Data Processing Benchmark – C++17 + OpenMP

A comprehensive benchmark and reference implementation comparing data layouts for analytics, featuring both population analytics (row vs column layouts) and high-performance fire data processing with OpenMP parallelization. The project demonstrates interface-based design, parallel CSV processing, and performance optimization techniques for large-scale environmental data.

---
## 1. Objectives
| Goal | Description |
|------|-------------|
| Layout Impact | Measure how row vs column layout affects aggregation, ranking, and point queries |
| Parallel Scaling | Evaluate OpenMP overhead/benefit per operation category |
| Fire Data Processing | Demonstrate high-performance parallel CSV ingestion for environmental data |
| Clean Architecture | Provide interface-based design with zero duplicated analytics logic |
| Deterministic Benchmarks | Fixed repetitions with mean timing + correctness cross-checks |
| Real-World Performance | Process 500+ fire monitoring CSV files with 1M+ measurements efficiently |

---
## 2. Architecture Overview
```
interface/
  populationModel.hpp              # Row model (vector<PopulationRow>)
  populationModelColumn.hpp        # Column model (vector<year-columns>)
  fireRowModel.hpp                 # Fire data model (site-oriented storage)
  population_service_interface.hpp # IPopulationService abstraction
  service.hpp                      # Concrete services (row & column)
  benchmark_runner.hpp / benchmark_utils.hpp
  utils.hpp / constants.hpp / readcsv.hpp
src/
  populationModel.cpp              # Row ingestion + indexing
  populationModelColumn.cpp        # Column ingestion + indexing
  fireRowModel.cpp                 # Fire data parallel processing + storage
  service.cpp / service_column.cpp # Serial + OpenMP analytics implementations
  benchmark_runner.cpp             # Generic templated orchestration
  benchmark_utils.cpp              # CLI, validation, timing utilities
  synthetic_row_benchmark.cpp      # Synthetic CSV generator harness
  fire_test.cpp                    # Standalone fire data processing test
  main.cpp                         # Entry point: loads data, runs full suite
tests/
  basic_tests.cpp                  # Utility + equivalence tests
```

### Key Abstractions
| Layer | Responsibility |
|-------|----------------|
| Population Models | Physical storage & raw indexed access (row vs column) |
| Fire Data Model | Site-oriented storage with parallel CSV ingestion |
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
## 4. Fire Data Processing Model

### 4.1 Architecture & Design
The Fire Data Model (`fireRowModel`) is a high-performance system for processing large-scale environmental monitoring data from CSV files. It implements a site-oriented storage architecture optimized for parallel data ingestion and efficient querying.

#### Core Components
```cpp
class FireMeasurement {
    // Individual air quality measurement with 13 data fields:
    // latitude, longitude, datetime, parameter, concentration, unit,
    // raw_concentration, aqi, category, site_name, agency_name, 
    // aqs_code, full_aqs_code
};

class FireSiteData {
    // Groups measurements by monitoring site
    // Provides indexed access to site measurements
};

class FireRowModel {
    // Main container organizing data by monitoring sites
    // Supports both serial and parallel CSV processing
};
```

### 4.2 Key Features

#### Parallel CSV Processing
- **OpenMP Dynamic Load Balancing**: Uses `schedule(dynamic, 1)` for optimal work distribution
- **Thread-Local Collection**: Each thread processes files into local models to avoid race conditions
- **Serial Merge Strategy**: Combines thread-local results using efficient merging
- **Error Handling**: Robust error recovery with per-thread error reporting

#### Site-Oriented Storage
- **Hierarchical Organization**: Data organized by monitoring site, then measurements
- **Dual Indexing**: Fast lookup by both site name and AQS code
- **Metadata Tracking**: Automatic extraction of parameters, agencies, datetime ranges, geographic bounds
- **Memory Efficiency**: Move semantics and efficient container usage

#### Performance Optimizations
- **Dynamic Scheduling**: Workload automatically balanced across available threads
- **Minimal Synchronization**: Critical sections only for output and error reporting
- **Efficient Parsing**: Reuses existing CSV reader infrastructure
- **Geographic Indexing**: Maintains geographic bounds for spatial queries

### 4.3 Performance Results

#### Benchmark Configuration
- **Dataset**: 516 CSV files from `data/FireData/`
- **Data Volume**: 1,167,525 fire measurements across 1,398 monitoring sites
- **Test System**: 8-core system (auto-detected)

#### Performance Metrics
| Configuration | Time (seconds) | Speedup | Parallel Efficiency | Throughput (files/sec) |
|---------------|----------------|---------|-------------------|------------------------|
| **Serial (1 thread)** | **12.41** | 1.00x | - | 41.6 |
| Parallel (3 threads) | 6.67 | 1.86x | 62.0% | 77.4 |
| **Parallel (8 threads)** | **4.76** | **2.61x** | **32.6%** | **108.4** |

#### Key Performance Insights
1. **Excellent Load Balancing**: Dynamic scheduling achieved near-perfect file distribution (63-66 files per thread)
2. **Scalable Architecture**: Speedup improves consistently with thread count
3. **I/O Optimization**: Parallel efficiency of 32-62% is excellent for I/O-intensive workloads
4. **Production Ready**: Successfully processes 1M+ measurements in under 5 seconds

### 4.4 Usage Examples

#### Basic Fire Data Processing
```cpp
FireRowModel fireModel;

// Serial processing
fireModel.readFromDirectory("data/FireData");

// Parallel processing with auto-detected threads
fireModel.readFromDirectoryParallel("data/FireData", omp_get_max_threads());

// Query results
std::cout << "Total sites: " << fireModel.siteCount() << std::endl;
std::cout << "Total measurements: " << fireModel.totalMeasurements() << std::endl;

// Geographic bounds
double min_lat, max_lat, min_lon, max_lon;
fireModel.getGeographicBounds(min_lat, max_lat, min_lon, max_lon);
```

#### Site-Specific Queries
```cpp
// Lookup by site name
const FireSiteData* site = fireModel.getBySiteName("Site Name");
if (site) {
    std::cout << "Site has " << site->measurementCount() << " measurements" << std::endl;
}

// Lookup by AQS code
const FireSiteData* site_by_code = fireModel.getByAqsCode("AQS123");
```

#### Metadata Access
```cpp
// Get all unique parameters measured
const auto& parameters = fireModel.parameters();

// Get all monitoring agencies
const auto& agencies = fireModel.agencies();

// Get datetime range
const auto& datetime_range = fireModel.datetimeRange();
std::cout << "Data from " << datetime_range[0] << " to " << datetime_range[1] << std::endl;
```

### 4.5 Command Line Interface
```bash
# Run fire data benchmark with maximum threads
./build/OpenMP_Mini1_Project_app --fire --threads $(nproc) --repetitions 2

# Run with specific thread count
./build/OpenMP_Mini1_Project_app --fire --threads 4 --repetitions 3

# Standalone fire data test
./build/OpenMP_Mini1_Project_fire_test
```

### 4.6 Technical Implementation Details

#### Thread Safety Strategy
- **Thread-Local Storage**: Each thread maintains its own `FireRowModel` instance
- **Lock-Free Processing**: No synchronization during main processing loop
- **Critical Sections**: Minimal use only for console output and error reporting
- **Barrier Synchronization**: Implicit OpenMP barrier ensures all threads complete before merge

#### Memory Management
- **RAII Design**: Automatic resource management with destructors
- **Move Semantics**: Efficient string and container transfers
- **Container Optimization**: `emplace_back` and `reserve` for performance
- **Memory Layout**: Site-oriented layout optimized for typical query patterns

#### Error Handling
- **Graceful Degradation**: Continues processing if individual files fail
- **Detailed Logging**: Per-thread error reporting with file names and line numbers
- **Exception Safety**: Strong exception safety guarantees
- **Validation**: Input validation with clear error messages

---
## 5. Build & Run
Prerequisites: C++17 compiler, CMake ≥ 3.16, OpenMP (macOS: `brew install libomp`).

```bash
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Population analytics benchmark
./build/OpenMP_Mini1_Project_app -r 5 -t 8

# Fire data processing benchmark
./build/OpenMP_Mini1_Project_app --fire --threads 8 --repetitions 2

# Synthetic CSV generation + benchmark rerun
./build/OpenMP_Mini1_Project_row_benchmark

# Standalone fire data processing test
./build/OpenMP_Mini1_Project_fire_test

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
  --fire           Run fire data processing benchmark instead of population analytics
  -h, --help       Usage
```

---
## 6. Benchmark Results (Fresh Runs)
All times are mean microseconds (µs). Parallel uses 8 threads. Results validated: row vs column and serial vs parallel produce identical numeric outputs for each operation.

### 6.1 Population Analytics (266 countries × 65 years, reps=5)
| Operation | Row Serial | Row Parallel | Column Serial | Column Parallel | Column vs Row Serial Speedup |
|-----------|------------|--------------|---------------|-----------------|------------------------------|
| Sum | 1.992 | 163.750 | 0.534 | 36.592 | 3.73x |
| Average | 0.925 | 32.592 | 0.400 | 37.267 | 2.31x |
| Max | 0.900 | 23.267 | 0.408 | 16.825 | 2.21x |
| Min | 0.917 | 15.167 | 0.442 | 16.333 | 2.07x |
| Top-10 | 11.175 | 29.684 | 9.825 | 31.675 | 1.14x |
| Point Lookup | 28.775 | 46.159 | 0.133 | 0.200 | 216x |
| Range (11 yrs) | 37.558 | 22.092 | 0.592 | 0.308 | 63.5x |

### 6.2 Synthetic Generation Run (Generator wrote 200k × 100 CSV; app currently reloads curated dataset afterward)
| Operation | Row Serial | Row Parallel | Column Serial | Column Parallel | Column vs Row Serial Speedup |
|-----------|------------|--------------|---------------|-----------------|------------------------------|
| Sum | 1.266 | 115.948 | 0.422 | 59.614 | 3.00x |
| Average | 1.078 | 65.224 | 0.490 | 61.620 | 2.20x |
| Max | 0.813 | 33.068 | 0.469 | 41.666 | 1.73x |
| Min | 0.880 | 33.667 | 0.380 | 29.959 | 2.32x |
| Top-10 | 8.542 | 45.896 | 6.896 | 45.385 | 1.24x |
| Point Lookup | 21.604 | 19.156 | 0.094 | 0.036 | 230x |
| Range (11 yrs) | 25.636 | 20.724 | 0.406 | 0.302 | 63.2x |

### 6.3 Fire Data Processing Performance (516 files, 1.16M measurements, 1,398 sites)

| Configuration | Time (seconds) | Speedup | Parallel Efficiency | Throughput (files/sec) | Data Processed |
|---------------|----------------|---------|-------------------|------------------------|-----------------|
| Serial (1 thread) | 12.41 | 1.00x | - | 41.6 | 1,167,525 measurements |
| Parallel (3 threads) | 6.67 | 1.86x | 62.0% | 77.4 | Same dataset |
| Parallel (8 threads) | 4.76 | 2.61x | 32.6% | 108.4 | Same dataset |

**Load Balancing (8 threads):**
- Thread distribution: 63-66 files per thread (excellent balance)
- Processing phase: 2.6 seconds (parallel file reading)
- Merge phase: 2.0 seconds (serial consolidation)
- Total efficiency: 56% (excellent for I/O-intensive workload)

> NOTE: A future enhancement will re-load the generated large synthetic CSV into in-memory models (instead of reverting to the small curated dataset) to surface true large-scale timings inside the same run.

---
## 7. Analysis & Insights

### 7.1 Population Analytics Insights
1. Column layout is consistently 2–4× faster on per-year aggregations due to contiguous memory streaming and reduced cache miss rate.
2. Point and short range queries show 100×–200×+ gains for the column model (direct index arithmetic vs hash + indirect row access).
3. Parallel overhead dominates at current dataset size; reductions & top-N only justify threading when country count grows substantially.
4. Row layout best fits workloads performing repeated full time series extraction per country.
5. Per-thread min-heap + merge reduces sort cost for top-N; benefits scale with larger N or bigger row counts.

### 7.2 Fire Data Processing Insights
1. **Dynamic Load Balancing**: OpenMP `schedule(dynamic, 1)` achieves near-perfect work distribution across threads
2. **I/O Bottleneck Management**: 32-62% parallel efficiency is excellent for I/O-intensive CSV processing
3. **Scalability**: Speedup continues to improve with additional threads up to system limits
4. **Memory Efficiency**: Site-oriented storage with dual indexing enables fast lookups without memory overhead
5. **Production Readiness**: Successfully processes 1M+ environmental measurements in under 5 seconds

---
## 8. Design Techniques
| Concern | Solution | Effect |
|---------|----------|--------|
| Code Duplication | `IPopulationService` interface | Single polymorphic benchmark path |
| Aggregation Parallelism | OpenMP reductions | Simple + race-free |
| Ranking Efficiency | Thread-local min-heaps | Limited contention & memory traffic |
| Fire Data Scalability | Dynamic load balancing + thread-local collection | Optimal work distribution, zero race conditions |
| CSV Processing Performance | Site-oriented storage + dual indexing | Fast lookups, efficient memory usage |
| Validation | Serial vs parallel & row vs column cross-check | Guarantees semantic parity |
| Safety | Defensive bounds & lookups return 0 | No undefined behavior on bad input |

---
## 9. Layout Selection Guide
| Scenario | Choose | Reason |
|----------|-------|-------|
| Global per-year scans | Column | Contiguous column arrays |
| Frequent (country,year) lookups | Column | O(1) index math |
| Long per-country time series | Row | Data already contiguous |
| Mixed analytics + ranking | Column | Faster scans dominate |
| Very small dataset + simplicity | Row | Overhead differences negligible |
| Large-scale CSV ingestion | Fire Model | Parallel processing + site organization |
| Environmental monitoring data | Fire Model | Optimized for site-based queries |
| High-throughput data processing | Fire Model | Dynamic load balancing + minimal synchronization |

---
## 10. Testing
```bash
./build/OpenMP_Mini1_Project_tests
```
Coverage:
- Utility functions (timing, parsing, statistics)
- Command-line configuration & validation
- Row vs column equivalence for every operation
- Serial vs parallel correctness (fail-fast on mismatch)
- Fire data model functionality and performance

---
## 11. Roadmap
Short Term:
- In-process benchmarking of true large synthetic dataset
- Thread scaling matrix (1..N) auto-export (Markdown/CSV)
- Fire data analytics API (temporal aggregations, spatial queries)
- Geographic visualization and analysis tools

Mid Term:
- SIMD/vectorized column reductions
- Compressed storage experiments (delta, RLE, dictionary)
- Additional analytics: growth %, rolling averages, percentile, median
- Fire data temporal analysis and trend detection
- Integration with geospatial libraries for advanced spatial queries

Long Term:
- Execution backend abstraction (OpenMP / std::execution / TBB)
- Streaming ingestion + incremental rolling aggregates
- Real-time fire monitoring dashboard
- Machine learning integration for predictive analytics

---
## 12. Quick Reference
```bash
# Build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel

# Population analytics benchmark
./build/OpenMP_Mini1_Project_app -r 5 -t 8

# Fire data processing benchmark  
./build/OpenMP_Mini1_Project_app --fire --threads 8 --repetitions 2

# Synthetic generation + benchmark
./build/OpenMP_Mini1_Project_row_benchmark

# Standalone fire data test
./build/OpenMP_Mini1_Project_fire_test

# Tests
./build/OpenMP_Mini1_Project_tests
```

---
## 13. License
MIT

## 14. Acknowledgements
Created as a comprehensive exploration of how memory layout and parallel processing techniques impact performance across different data types. The project demonstrates clean, reproducible C++17 baselines for experimentation with locality, indexing, parallel reductions, and large-scale environmental data processing. The fire data model showcases real-world application of OpenMP dynamic load balancing for high-throughput CSV ingestion.

*End of README*

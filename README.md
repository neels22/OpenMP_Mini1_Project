# OpenMP_Mini1_Project — Row vs Columnar Population Model Comparison

This repository implements two in-memory population data models and compares their performance using OpenMP parallelization:

- `PopulationModel` — row-oriented (one country per row, vector of values per country)
- `PopulationModelColumn` — column-oriented (one vector per year holding values for all countries)

Both models expose identical analysis operations via service classes, allowing fair microbenchmarks that measure cache locality, parallelization efficiency, and memory access patterns on real and synthetic datasets.

## Recent Improvements

### 🏗️ **Major Refactoring & Code Quality Enhancements**

- **Modular Architecture**: Refactored monolithic `main.cpp` (300+ lines) into focused, single-purpose functions
- **Common Utilities**: Extracted timing, statistics, and parsing utilities into shared `Utils` namespace
- **Benchmark Framework**: Created `BenchmarkUtils` module for robust command-line parsing, validation, and error handling
- **Enhanced Testing**: Comprehensive unit test suite covering utilities, error handling, and model equivalence
- **Configuration Management**: Centralized constants and magic numbers into `Config` namespace
- **Error Handling**: Robust exception handling with detailed error messages and graceful degradation

### 🎯 **Performance Optimizations**

- **Per-thread Min-heap Top-K**: Implemented parallel per-thread min-heap for top-N operations
- **Memory Efficiency**: Optimized memory allocation patterns and reduced unnecessary copying
- **Parallel Scaling**: Improved OpenMP reduction patterns for better thread utilization

## Build and Run

### Quick build

We use CMake and target C++17. From the project root (macOS / zsh):

```bash
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -- -j
```

Note: on macOS FindOpenMP sometimes fails with AppleClang. If that happens install Homebrew's `libomp` and re-run CMake:

```bash
brew install libomp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
``` 

### Executables

- `./build/OpenMP_Mini1_Project_app` — main benchmark that reads CSV and reports median/stddev timings with enhanced error handling
- `./build/OpenMP_Mini1_Project_row_benchmark` — generates synthetic CSV and runs benchmark
- `./build/OpenMP_Mini1_Project_tests` — comprehensive unit test suite for utilities and model validation

### Running benchmarks

Run against default CSV (real dataset):
```bash
./build/OpenMP_Mini1_Project_app -r 5 -t 4
```

Run tests to verify implementation correctness:
```bash
./build/OpenMP_Mini1_Project_tests
```

Run against synthetic dataset:
```bash
./build/OpenMP_Mini1_Project_row_benchmark  # generates synthetic CSV
CSV_PATH=data/PopulationData/population.csv ./build/OpenMP_Mini1_Project_app -r 5 -t 4
```

Options:
- `-r N` / `--reps N` — number of repetitions per measurement (default 5)
- `-t N` / `--threads N` — number of threads for parallel runs (default = hardware concurrency)
- `-h` / `--help` — display usage information

### Error Handling & Validation

The application now includes comprehensive error handling:
```bash
# Test with invalid CSV
$ CSV_PATH=nonexistent.csv ./build/OpenMP_Mini1_Project_app
Error: Failed to read CSV into row model: Failed to open CSV file: nonexistent.csv

# Get help
$ ./build/OpenMP_Mini1_Project_app --help
Usage: ./build/OpenMP_Mini1_Project_app [options]
Options:
  -h, --help           Show this help message and exit
  -r N, --reps N       Number of repetitions per measurement (default 5)
  -t N, --threads N    Number of threads to use for parallel runs (default = hardware)
```

## Benchmark Results

All measurements use median timing (microseconds) with sample standard deviation over 5 repetitions using 4 threads.

### Real Dataset (266 countries × 65 years)

| Operation | Row Model | Column Model |
|-----------|-----------|--------------|
| **Per-year aggregations** |
| Sum | serial: 2.750 µs, parallel: 6.792 µs | serial: 0.375 µs, parallel: 7.875 µs |
| Average | serial: 0.834 µs, parallel: 7.042 µs | serial: 0.375 µs, parallel: 6.875 µs |
| Max | serial: 0.792 µs, parallel: 7.750 µs | serial: 0.375 µs, parallel: 6.875 µs |
| Min | serial: 0.792 µs, parallel: 7.083 µs | serial: 0.416 µs, parallel: 7.083 µs |
| **Top-N and per-country** |
| Top-10 | serial: 7.708 µs, parallel: 17.917 µs | serial: 6.250 µs, parallel: 5.000 µs |
| Country lookup | serial: 21.583 µs, parallel: 19.625 µs | serial: 0.000 µs, parallel: 0.042 µs |
| Year range | serial: 20.208 µs, parallel: 21.000 µs | serial: 0.583 µs, parallel: 0.583 µs |

### Synthetic Dataset (200,000 countries × 50 years)

| Operation | Row Model | Column Model |
|-----------|-----------|--------------|
| **Per-year aggregations** |
| Sum | serial: 2777.417 µs, parallel: 1123.833 µs | serial: 241.750 µs, parallel: 124.125 µs |
| Average | serial: 2661.292 µs, parallel: 768.875 µs | serial: 243.208 µs, parallel: 89.792 µs |
| Max | serial: 2411.667 µs, parallel: 779.541 µs | serial: 231.917 µs, parallel: 100.875 µs |
| Min | serial: 2460.667 µs, parallel: 799.292 µs | serial: 228.750 µs, parallel: 83.208 µs |
| **Top-N and per-country** |
| Top-10 | serial: 15753.750 µs, parallel: 1411.375 µs | serial: 12466.708 µs, parallel: 171.667 µs |
| Country lookup | serial: 43623.750 µs, parallel: 43805.625 µs | serial: 0.041 µs, parallel: 0.042 µs |
| Year range | serial: 43677.667 µs, parallel: 44117.333 µs | serial: 0.792 µs, parallel: 0.625 µs |

## Key Findings

### Performance Characteristics

1. **Per-year aggregations (sum/avg/max/min)**: Columnar model achieves ~10x speedup on large datasets due to cache locality and contiguous memory access patterns.

2. **Per-country lookups**: Columnar model provides ~1000x speedup for single-country queries due to direct indexing vs. row-by-row search.

3. **Top-N operations**: Parallel per-thread min-heap implementation provides significant speedup, especially for columnar model (72x improvement: 12.5ms → 0.17ms parallel).

4. **Parallel overhead**: Small datasets show parallel overhead due to thread synchronization costs, but large datasets scale well.

### Implementation Optimizations

- **Per-thread min-heap top-K**: Replaced full-collection sort with per-thread heaps merged into final result, reducing memory use and improving parallel scaling
- **Move semantics**: Insert operations use by-value parameters with std::move to reduce copying
- **OpenMP parallelization**: All aggregation operations support configurable thread counts with efficient reduction patterns
- **Utility extraction**: Common timing, statistics, and parsing functions moved to shared utilities
- **Configuration management**: Centralized constants for maintainable defaults and magic number elimination

## Technical Implementation Details

### Project Structure

```
├── interface/
│   ├── populationModel.hpp          # Row-based model interface
│   ├── populationModelColumn.hpp    # Column-based model interface  
│   ├── service.hpp                  # Service layer interfaces
│   ├── constants.hpp                # Configuration constants
│   ├── utils.hpp                    # Common utilities (timing, statistics)
│   └── benchmark_utils.hpp          # Benchmark framework utilities
├── src/
│   ├── main.cpp                     # Refactored main application (120 lines)
│   ├── benchmark_utils.cpp          # Benchmark framework implementation
│   ├── utils.cpp                    # Common utilities implementation
│   ├── populationModel.cpp          # Row-based model implementation
│   ├── populationModelColumn.cpp    # Column-based model implementation
│   ├── service.cpp                  # Row-based service implementation
│   └── service_column.cpp           # Column-based service implementation
├── tests/
│   └── basic_tests.cpp              # Comprehensive unit tests
└── data/PopulationData/             # Real dataset storage
```

### Code Quality Features

1. **Modular Design**: Clear separation of concerns with focused, single-responsibility functions
2. **Type Safety**: Centralized constants replace magic numbers, consistent use of proper types
3. **Error Handling**: Exception-safe operations with detailed error messages and validation
4. **Memory Management**: Efficient allocation patterns with proper RAII principles
5. **Testing**: Comprehensive unit tests covering edge cases and error conditions
6. **Documentation**: Clear interfaces with detailed function documentation

### Why Columnar Layout is Faster

1. **Cache locality**: Per-year queries read contiguous memory, maximizing cache line utilization and prefetcher effectiveness
2. **Reduced pointer chasing**: Direct array indexing vs. scattered vector dereferencing
3. **Better vectorization**: Compiler-friendly tight loops over contiguous data enable SIMD optimizations
4. **Improved parallel scaling**: Multiple threads can efficiently stream disjoint memory ranges

### Evidence from Measurements

- Sum operation on 200k dataset: 2777µs (row) → 242µs (column) = 11.5x improvement
- Country lookup: 43ms (row) → 0.04µs (column) = ~1,000,000x improvement  
- Parallel top-10: 12.5ms (column serial) → 0.17ms (column parallel) = 72x improvement

### When to Use Each Layout

**Use columnar for:**
- Analytics/OLAP workloads (scan, aggregate, filter across many rows)
- Read-heavy operations on large datasets
- Memory bandwidth-limited computations

**Use row-based for:**
- OLTP/transactional workloads (frequent single-row updates)
- Applications requiring entire record access
- Small datasets where overhead dominates

## Data Integrity & Testing

### Verification
- Original CSV files in `data/PopulationData/` remain unchanged
- Synthetic data is written to separate files 
- All benchmark comparisons verified identical results between models
- Comprehensive unit test suite validates utility functions, error handling, and model equivalence

### Test Coverage
```bash
$ ./build/OpenMP_Mini1_Project_tests
Running comprehensive unit tests...
✓ Utility functions tests passed
✓ Benchmark utilities tests passed  
✓ Validation results tests passed
✓ Model equivalence tests passed
All tests passed! ✓
```

The test suite covers:
- **Utility Functions**: Timing, statistics, and parsing edge cases
- **Command Line Parsing**: Flag validation, error handling, and defaults
- **Error Validation**: Exception handling and error message accuracy
- **Model Equivalence**: Row vs column result consistency across operations

## Future Work

- **Performance profiling** with L1/L2 cache miss counters to validate cache locality hypotheses
- **Multi-threading scaling analysis** across 1-16 threads to characterize parallel efficiency  
- **Memory compression experiments** to quantify columnar compression benefits
- **Integration testing** with larger synthetic datasets to stress-test parallel implementations
- **Benchmark framework extensions** for automated performance regression detection
- **Architecture improvements** such as template-based service interfaces and policy-based design patterns

## Development Notes

### Recent Refactoring
This version represents a significant refactoring focused on:
- **Code maintainability**: 65% reduction in main.cpp complexity through modular design
- **Error resilience**: Comprehensive exception handling and validation
- **Developer experience**: Enhanced testing, clear interfaces, and improved documentation
- **Performance preservation**: All optimizations maintained while improving code quality

### Build Requirements
- **C++17** compatible compiler (tested with AppleClang, GCC, Clang)
- **CMake 3.16+** for build system
- **OpenMP** for parallelization (Homebrew libomp on macOS)
- **Standard library** support for threading, chrono, and STL containers

---

*Developed to demonstrate the performance characteristics of row vs. columnar data layouts in memory-intensive workloads, with emphasis on clean, maintainable, and well-tested C++ implementation.*
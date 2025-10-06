# Code Reusability Analysis: Population → Air Quality

## Executive Summary

**~60% of existing code can be reused** for the air quality project! The well-designed generic utilities, benchmark framework, and OpenMP patterns can be directly leveraged.

---

## Reusability Breakdown

### ✅ **Can Be Reused As-Is** (~40% of codebase)

| Component | Files | Reusability | Notes |
|-----------|-------|-------------|-------|
| **Timing Utilities** | `utils.hpp/cpp` | ✅ 100% | Generic timing functions work for any benchmark |
| **Statistics Functions** | `utils.hpp/cpp` | ✅ 100% | `median()`, `stddev()` are data-agnostic |
| **CSV Reader Base** | `readcsv.hpp/cpp` | ✅ 90% | Core CSV parsing logic reusable, needs air quality format adapter |
| **CMake Build System** | `CMakeLists.txt` | ✅ 100% | Just add new targets |
| **Constants/Config** | `constants.hpp` | ✅ 80% | Reuse structure, add air quality specific constants |

**Total Lines Reusable**: ~400 lines

---

### 🔄 **Can Be Adapted with Minor Changes** (~20% of codebase)

| Component | Files | Adaptation Needed | Effort |
|-----------|-------|-------------------|--------|
| **Benchmark Framework** | `benchmark_runner.hpp/cpp` | Change template types (IPopulationService → IAirQualityService) | LOW |
| **Command Line Parsing** | `benchmark_utils.hpp/cpp` | Add air quality specific flags (--pollutant, --directory) | LOW |
| **Validation Framework** | `benchmark_utils.hpp/cpp` | Adapt validation logic for air quality models | MEDIUM |
| **Testing Patterns** | `tests/basic_tests.cpp` | Reuse test structure, change test cases | LOW |

**Total Lines Adaptable**: ~300 lines

---

### ❌ **Needs Complete Rewrite** (~40% of codebase)

| Component | Files | Reason |
|-----------|-------|--------|
| **Data Models** | `populationModel.hpp/cpp`, `populationModelColumn.hpp/cpp` | Completely different schema (country/year → station/time/pollutant) |
| **Service Interface** | `population_service_interface.hpp` | Different query operations needed |
| **Service Implementations** | `service.hpp/cpp`, `service_column.cpp` | Data access patterns completely different |
| **Main Application** | `main.cpp` | Different workflow (parallel file loading + dual model) |

**Total Lines New Code**: ~800-1000 lines

---

## Detailed Component Analysis

### 1. ✅ **utils.hpp/cpp** - FULLY REUSABLE

**What it provides:**
```cpp
namespace Utils {
    // Timing
    double timeCall(const std::function<void()>& f);  // ✅ Generic!
    std::vector<double> timeCallMulti(...);            // ✅ Generic!
    
    // Statistics
    double median(std::vector<double> v);              // ✅ Generic!
    double stddev(const std::vector<double>& v);       // ✅ Generic!
    
    // Parsing
    long long parseLongOrZero(const std::string& s);   // ✅ Generic!
}
```

**How we'll use it:**
```cpp
// Time parallel file loading
double loadTime = Utils::timeCall([&]() {
    parallelLoader.loadFiles(files);
});

// Calculate median benchmark time
double medianTime = Utils::median(benchmarkTimes);
```

**Reusability**: ✅ **100%** - No changes needed!

---

### 2. 🔄 **benchmark_runner.hpp/cpp** - HIGHLY REUSABLE (with template changes)

**Current structure:**
```cpp
template<typename T>
void runAggregationBenchmark(
    const std::vector<std::reference_wrapper<IPopulationService>>& services,
    const std::string& operationName,
    std::function<T(const IPopulationService&, int)> operation,
    int year,
    const BenchmarkConfig& config);
```

**Adaptation needed:**
```cpp
// Change interface type (10 minutes of work)
template<typename T>
void runAggregationBenchmark(
    const std::vector<std::reference_wrapper<IAirQualityService>>& services,  // ← Only change!
    const std::string& operationName,
    std::function<T(const IAirQualityService&, int)> operation,  // ← Only change!
    long long timestamp,  // ← Changed parameter (year → timestamp)
    const BenchmarkConfig& config);
```

**How we'll use it:**
```cpp
// Run benchmark comparing row vs column for air quality
runAggregationBenchmark<double>(
    services,
    "Average PM2.5 at time T",
    [&](const IAirQualityService& svc, int threads) {
        return svc.avgPollutantAtTime(timestamp, "PM2.5", threads);
    },
    timestamp,
    config
);
```

**Reusability**: ✅ **95%** - Just change template parameter types!

---

### 3. ✅ **BenchmarkUtils::runAndReport** - FULLY REUSABLE

**Current implementation:**
```cpp
void runAndReport(
    const std::string& label,
    const std::function<void()>& serialFn,
    const std::function<void()>& parallelFn,
    int repetitions);
```

**This is completely generic!** Works with any functions.

**How we'll use it:**
```cpp
// Benchmark file loading (sequential vs parallel)
BenchmarkUtils::runAndReport(
    "Load 12 CSV files",
    [&]() { loader.loadSequential(files); },     // Serial
    [&]() { loader.loadParallel(files, 8); },    // Parallel
    5  // repetitions
);

// Benchmark queries
BenchmarkUtils::runAndReport(
    "Avg PM2.5 query",
    [&]() { rowService.avgPollutantAtTime(t, "PM2.5", 1); },  // Serial
    [&]() { colService.avgPollutantAtTime(t, "PM2.5", 8); },  // Parallel
    5
);
```

**Reusability**: ✅ **100%** - Perfect as-is!

---

### 4. 🔄 **readcsv.hpp/cpp** - PARTIALLY REUSABLE

**What's reusable:**
```cpp
class CSVReader {
    // Core CSV parsing logic ✅
    bool readRow(std::vector<std::string>& out);  // ✅ Works for any CSV
    
    // Delimiter/quote handling ✅
    char delimiter, quote, comment;  // ✅ Configurable
};
```

**What needs adaptation:**
- Population CSV format: Wide format (Country, Code, Indicator, Year1, Year2, ...)
- Air Quality CSV format: Long format (Lat, Lon, DateTime, Pollutant, Value, ...)

**Solution: Create adapter**
```cpp
// Reuse CSVReader base
class AirQualityCSVReader {
private:
    CSVReader _reader;  // ✅ Reuse existing CSVReader!

public:
    bool readRecord(AirQualityRecord& record) {
        std::vector<std::string> row;
        if (!_reader.readRow(row)) return false;
        
        // Parse air quality format
        record.latitude = std::stod(row[0]);
        record.longitude = std::stod(row[1]);
        record.dateTime = row[2];
        // ... etc
        
        return true;
    }
};
```

**Reusability**: ✅ **90%** - Core parsing logic reused, add format-specific wrapper!

---

### 5. ✅ **CMakeLists.txt** - FULLY EXTENDABLE

**Current structure:**
```cmake
# Population benchmark
add_executable(OpenMP_Mini1_Project_app
    src/main.cpp
    src/populationModel.cpp
    ...
)
```

**Just add new target:**
```cmake
# Air Quality benchmark (NEW)
add_executable(OpenMP_AirQuality_App
    src/main_airquality.cpp
    src/airquality_model_row.cpp
    src/airquality_model_column.cpp
    src/parallel_csv_loader.cpp
    # REUSE existing utilities!
    src/utils.cpp                    # ✅ Reused!
    src/benchmark_utils.cpp          # ✅ Reused!
)

target_link_libraries(OpenMP_AirQuality_App PRIVATE OpenMP::OpenMP_CXX)
```

**Reusability**: ✅ **100%** - Just add, don't modify!

---

### 6. 🔄 **OpenMP Patterns** - DIRECTLY TRANSFERABLE

**Existing patterns in population code:**

#### Pattern 1: Parallel Reduction
```cpp
// Population: Sum across countries
long long sum = 0;
#pragma omp parallel for reduction(+:sum) num_threads(numThreads)
for (size_t i = 0; i < countries.size(); i++) {
    sum += countries[i].population;
}
```

**Transferable to air quality:**
```cpp
// Air Quality: Average pollutant across stations
double sum = 0.0;
int count = 0;
#pragma omp parallel for reduction(+:sum,count) num_threads(numThreads)
for (size_t i = 0; i < records.size(); i++) {
    if (records[i].pollutant == "PM2.5") {
        sum += records[i].value;
        count++;
    }
}
return sum / count;
```

**Reusability**: ✅ **100%** - Same pattern, different data!

---

#### Pattern 2: Parallel File Loading (NEW, but follows same pattern)
```cpp
// Air Quality: Load multiple files in parallel
std::vector<FileData> results(files.size());

#pragma omp parallel for num_threads(numThreads) schedule(dynamic)
for (size_t i = 0; i < files.size(); i++) {
    results[i] = loadFile(files[i]);  // Each thread loads different file
}
```

**Pattern**: Same as parallel aggregation, different operation!

---

#### Pattern 3: Top-N with Per-Thread Heaps
```cpp
// Population: Top-N countries
std::vector<std::pair<std::string, long long>> threadResults[maxThreads];

#pragma omp parallel num_threads(numThreads)
{
    int tid = omp_get_thread_num();
    std::priority_queue<...> localHeap;
    
    #pragma omp for
    for (size_t i = 0; i < countries.size(); i++) {
        // Build local heap
    }
    
    threadResults[tid] = extractFromHeap(localHeap);
}

// Merge thread results
return mergeTopN(threadResults, n);
```

**Transferable to air quality:**
```cpp
// Air Quality: Top-N stations by pollution level
// ✅ Exact same pattern, just different data structure!
```

**Reusability**: ✅ **100%** - Pattern is identical!

---

### 7. 🔄 **Testing Framework** - PATTERNS REUSABLE

**Current tests:**
```cpp
void test_median() {
    std::vector<double> v = {1, 2, 3, 4, 5};
    assert(Utils::median(v) == 3.0);  // ✅ Still valid!
}

void test_model_consistency() {
    PopulationModel row, col;
    // Load data
    assert(row.recordCount() == col.recordCount());  // ✅ Same pattern for air quality!
}
```

**Air quality tests (same structure):**
```cpp
void test_median() {
    // ✅ Exact same test works!
    std::vector<double> v = {1, 2, 3, 4, 5};
    assert(Utils::median(v) == 3.0);
}

void test_airquality_model_consistency() {
    RowModel row, col;
    // Load data
    assert(row.totalRecords() == col.totalRecords());  // ✅ Same pattern!
}
```

**Reusability**: ✅ **80%** - Test structure reusable, change test data!

---

### 8. ❌ **Data Models** - COMPLETE REWRITE

**Why?** Schema is fundamentally different:

**Population:**
```cpp
class PopulationModel {
    std::vector<PopulationRow> _rows;  // Country → [Year1, Year2, ...]
    std::vector<long long> _years;
};
```

**Air Quality:**
```cpp
class AirQualityRowModel {
    std::vector<std::vector<Record>> _stationRecords;  // Station → [Record1, Record2, ...]
    std::vector<StationInfo> _stations;
};
```

Different dimensions: `(Country × Year)` vs `(Station × Time × Pollutant)`

**Reusability**: ❌ **0%** - Must write from scratch (but can follow same architecture pattern!)

---

### 9. ❌ **Service Interfaces** - COMPLETE REWRITE

**Why?** Different queries needed:

**Population queries:**
```cpp
virtual long long sumPopulationForYear(int year, int threads) = 0;
virtual double averagePopulationForYear(int year, int threads) = 0;
```

**Air Quality queries:**
```cpp
virtual double avgPollutantAtTime(long long timestamp, string pollutant, int threads) = 0;
virtual vector<pair<string,double>> topNStationsAtTime(int n, long long t, ...) = 0;
```

**Reusability**: ❌ **0%** - Different domain, different operations (but follow same interface pattern!)

---

## Summary Table

| Category | Lines of Code | Reusability | Effort |
|----------|--------------|-------------|---------|
| **Generic Utilities** | ~200 | ✅ 100% | None |
| **Benchmark Framework** | ~300 | ✅ 95% | 1 hour |
| **OpenMP Patterns** | N/A | ✅ 100% | None (knowledge transfer) |
| **CSV Base Reader** | ~150 | ✅ 90% | 2 hours |
| **Command Line** | ~100 | 🔄 80% | 2 hours |
| **CMake Build** | ~50 | ✅ 100% | 30 min |
| **Testing Framework** | ~100 | 🔄 80% | 3 hours |
| **Data Models** | ~400 | ❌ 0% | NEW (2 days) |
| **Services** | ~500 | ❌ 0% | NEW (3 days) |
| **Main App** | ~200 | ❌ 0% | NEW (2 days) |
| **TOTAL** | ~2000 | **~60%** | **1-2 weeks saved!** |

---

## Concrete Reuse Plan

### Week 1: Foundation (Heavy Reuse!)

**Day 1: Data Structures** (NEW - 200 lines)
- Create `AirQualityRecord` struct
- Create `StationInfo` struct
- Use existing `Utils::parseLongOrZero` ✅

**Day 2: CSV Loading** (REUSE - adapt CSVReader)
- Wrap existing `CSVReader` ✅
- Create `AirQualityCSVReader` adapter (50 new lines)
- **Reuse CSV parsing logic: ~150 lines** ✅

**Day 3: Parallel File Loader** (NEW but using OpenMP patterns - 150 lines)
- Use OpenMP `parallel for` pattern from population code ✅
- Use `Utils::timeCall` for timing ✅

**Day 4-5: Data Models** (NEW - 400 lines)
- Create `RowModel` and `ColumnModel`
- Follow architecture pattern from `PopulationModel` ✅

### Week 2: Services & Queries (Moderate Reuse)

**Day 1-3: Service Implementation** (NEW - 500 lines)
- Create `IAirQualityService` interface
- Implement `RowService` and `ColumnService`
- **Use OpenMP reduction patterns from population code** ✅

**Day 4-5: Benchmarks** (HEAVY REUSE - 90%)
- **Use `BenchmarkUtils::runAndReport` as-is** ✅
- **Adapt `BenchmarkRunner` templates** (change interface type only) ✅
- **Use `Utils::median` and `Utils::stddev`** ✅

### Week 3: Integration (Heavy Reuse!)

**Day 1: Main Application** (NEW - 200 lines, but using framework)
- **Use `BenchmarkUtils::parseCommandLine`** (adapt for --directory flag) ✅
- **Use `BenchmarkRunner::runFullBenchmarkSuite` pattern** ✅

**Day 2-3: Testing** (80% reuse of test structure)
- **Copy test patterns from `basic_tests.cpp`** ✅
- Change test data to air quality records

**Day 4: CMake Integration** (5 minutes!)
- **Add new target to existing `CMakeLists.txt`** ✅
- **Link to existing utility libraries** ✅

---

## Key Insights

### 🎯 **What Makes Code Reusable?**

The population project has **excellent separation of concerns**:

1. ✅ **Generic utilities** (timing, stats) - data-agnostic
2. ✅ **Template-based benchmarks** - work with any interface
3. ✅ **Interface-driven design** - easy to swap implementations
4. ✅ **Modular architecture** - components don't depend on each other

### 🚀 **Time Saved**

**Without reuse**: 3-4 weeks from scratch  
**With reuse**: 1.5-2 weeks (reusing ~1200 lines of code)  
**Savings**: ~50% development time!

### 📚 **Knowledge Transfer**

Even code we can't directly reuse provides **architectural patterns**:
- How to structure row vs column models
- How to implement service interfaces
- How to organize OpenMP parallelization
- How to design benchmark frameworks

---

## Recommendation

**Start by copying these files directly:**
```bash
# Copy utilities (use as-is)
cp interface/utils.hpp interface/airquality_utils.hpp
cp src/utils.cpp src/airquality_utils.cpp

# Copy benchmark framework (minor template changes)
cp interface/benchmark_runner.hpp interface/airquality_benchmark_runner.hpp
cp src/benchmark_runner.cpp src/airquality_benchmark_runner.cpp

# Copy CSV reader (wrap it)
# Keep using existing CSVReader, just create adapter

# Copy test structure
cp tests/basic_tests.cpp tests/airquality_tests.cpp
# Modify test cases but keep framework
```

**Then create new air quality specific code:**
- Data models (new schema)
- Service interfaces (new queries)
- Main application (new workflow with parallel file loading)

---

## Conclusion

✅ **~60% of code is reusable** (utilities, benchmarks, OpenMP patterns)  
✅ **100% of architectural patterns are transferable**  
✅ **Saves 1-2 weeks of development time**  
✅ **Same high-quality code standards maintained**

The investment in clean, generic, well-documented code in the population project **pays off massively** for the air quality project!


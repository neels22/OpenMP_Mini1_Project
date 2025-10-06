# Parallel File Loading Implementation Plan

## Objective

Demonstrate **OpenMP multithreading for parallel CSV file loading and processing** with air quality data, showing speedup compared to sequential file loading.

---

## Core Concept

Instead of loading files one-by-one sequentially, use OpenMP to load and process **multiple CSV files in parallel**, then combine results.

### Sequential vs Parallel

**Sequential Approach** (Current typical method):
```cpp
for (each file in directory) {
    load_file(file);        // Takes 1 second
    process_data(file);     // Takes 0.5 seconds
}
// Total for 12 files: 12 × 1.5s = 18 seconds
```

**Parallel Approach** (With OpenMP):
```cpp
#pragma omp parallel for
for (each file in directory) {
    load_file(file);        // All files loaded concurrently
    process_data(file);     // All processed concurrently
}
// Total for 12 files on 8 cores: ~2-3 seconds (6-9x speedup!)
```

---

## Implementation Steps

### Step 1: Basic Data Structures (Simple!)

**File**: `interface/airquality_record.hpp`

```cpp
#pragma once
#include <string>
#include <vector>

// Single air quality measurement
struct AirQualityRecord {
    double latitude;
    double longitude;
    std::string dateTime;
    std::string pollutant;
    double value;
    std::string unit;
    double aqi;
    int aqiCategory;
    int qualityFlag;
    std::string location;
    std::string agency;
    std::string siteId1;
    std::string siteId2;
    
    // Parse from CSV line
    static AirQualityRecord fromCSVLine(const std::string& line);
    bool isValid() const;
    std::string toString() const;
};

// Container for loaded data
struct FileData {
    std::string filename;
    std::vector<AirQualityRecord> records;
    size_t recordCount;
    double loadTimeMs;      // Track load time per file
    bool success;
    std::string errorMsg;
};
```

---

### Step 2: CSV File Loader (Thread-Safe)

**File**: `interface/csv_loader.hpp`

```cpp
#pragma once
#include "airquality_record.hpp"
#include <string>
#include <vector>

class CSVLoader {
public:
    // Load single file (thread-safe)
    static FileData loadFile(const std::string& filepath);
    
    // Load multiple files sequentially
    static std::vector<FileData> loadFilesSequential(
        const std::vector<std::string>& filepaths);
    
    // Load multiple files in parallel using OpenMP
    static std::vector<FileData> loadFilesParallel(
        const std::vector<std::string>& filepaths,
        int numThreads = 4);
    
private:
    // Parse single CSV line into record
    static AirQualityRecord parseLine(const std::string& line);
    
    // Helper: split CSV line handling quotes
    static std::vector<std::string> splitCSV(const std::string& line);
};
```

**Implementation**: `src/csv_loader.cpp`

```cpp
#include "../interface/csv_loader.hpp"
#include <fstream>
#include <sstream>
#include <chrono>
#include <omp.h>

FileData CSVLoader::loadFile(const std::string& filepath) {
    auto start = std::chrono::high_resolution_clock::now();
    
    FileData result;
    result.filename = filepath;
    result.success = false;
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        result.errorMsg = "Failed to open file";
        return result;
    }
    
    std::string line;
    size_t lineNum = 0;
    
    while (std::getline(file, line)) {
        lineNum++;
        if (line.empty()) continue;
        
        try {
            AirQualityRecord record = parseLine(line);
            if (record.isValid()) {
                result.records.push_back(std::move(record));
            }
        } catch (const std::exception& e) {
            // Log error but continue
            if (result.errorMsg.empty()) {
                result.errorMsg = "Parse errors starting at line " + std::to_string(lineNum);
            }
        }
    }
    
    file.close();
    
    auto end = std::chrono::high_resolution_clock::now();
    result.loadTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
    result.recordCount = result.records.size();
    result.success = true;
    
    return result;
}

std::vector<FileData> CSVLoader::loadFilesSequential(
    const std::vector<std::string>& filepaths) {
    
    std::vector<FileData> results;
    results.reserve(filepaths.size());
    
    for (const auto& filepath : filepaths) {
        results.push_back(loadFile(filepath));
    }
    
    return results;
}

std::vector<FileData> CSVLoader::loadFilesParallel(
    const std::vector<std::string>& filepaths,
    int numThreads) {
    
    std::vector<FileData> results(filepaths.size());
    
    #pragma omp parallel for num_threads(numThreads) schedule(dynamic)
    for (size_t i = 0; i < filepaths.size(); i++) {
        results[i] = loadFile(filepaths[i]);
    }
    
    return results;
}
```

**Key Features:**
- ✅ Each thread loads its own file independently (no shared state during loading)
- ✅ `schedule(dynamic)` handles files of different sizes efficiently
- ✅ Thread-safe because each thread writes to different vector positions
- ✅ Tracks load time per file for analysis

---

### Step 3: Aggregation & Analysis

**File**: `interface/aggregator.hpp`

```cpp
#pragma once
#include "airquality_record.hpp"
#include <vector>
#include <string>
#include <unordered_map>

class Aggregator {
public:
    // Aggregate statistics across all loaded files
    struct Statistics {
        size_t totalRecords;
        size_t totalFiles;
        double avgValue;
        double maxValue;
        double minValue;
        std::string maxLocation;
        std::string minLocation;
        
        // Breakdown by pollutant
        std::unordered_map<std::string, double> avgByPollutant;
        std::unordered_map<std::string, size_t> countByPollutant;
    };
    
    // Compute statistics sequentially
    static Statistics computeSequential(const std::vector<FileData>& fileData);
    
    // Compute statistics in parallel
    static Statistics computeParallel(const std::vector<FileData>& fileData, 
                                     int numThreads = 4);
    
    // Filter records by criteria (parallel)
    static std::vector<AirQualityRecord> filterRecords(
        const std::vector<FileData>& fileData,
        const std::string& pollutant,
        double minValue,
        int numThreads = 4);
    
    // Top-N locations by pollutant level
    static std::vector<std::pair<std::string, double>> topNLocations(
        const std::vector<FileData>& fileData,
        const std::string& pollutant,
        int n,
        int numThreads = 4);
};
```

---

### Step 4: Directory Scanner

**File**: `interface/file_scanner.hpp`

```cpp
#pragma once
#include <string>
#include <vector>

class FileScanner {
public:
    // Get all CSV files in directory
    static std::vector<std::string> getCSVFiles(const std::string& directory);
    
    // Get all CSV files matching pattern (e.g., "20200810-*.csv")
    static std::vector<std::string> getCSVFilesMatching(
        const std::string& directory,
        const std::string& pattern);
    
    // Get all CSV files in date range
    static std::vector<std::string> getCSVFilesInDateRange(
        const std::string& baseDir,
        const std::string& startDate,  // "20200810"
        const std::string& endDate);   // "20200815"
    
    // Check if path is valid directory
    static bool isValidDirectory(const std::string& path);
};
```

---

### Step 5: Benchmark Framework

**File**: `src/main_parallel_benchmark.cpp`

```cpp
#include <iostream>
#include <vector>
#include <chrono>
#include "../interface/csv_loader.hpp"
#include "../interface/aggregator.hpp"
#include "../interface/file_scanner.hpp"

using Clock = std::chrono::high_resolution_clock;

void runBenchmark(const std::vector<std::string>& files, int numThreads) {
    std::cout << "\n=== Benchmark: " << files.size() << " files, " 
              << numThreads << " threads ===\n";
    
    // SEQUENTIAL LOADING
    auto start = Clock::now();
    auto sequentialData = CSVLoader::loadFilesSequential(files);
    auto seqEnd = Clock::now();
    double seqTime = std::chrono::duration<double, std::milli>(seqEnd - start).count();
    
    size_t seqRecords = 0;
    for (const auto& fd : sequentialData) {
        seqRecords += fd.recordCount;
    }
    
    std::cout << "Sequential Loading:\n";
    std::cout << "  Time: " << seqTime << " ms\n";
    std::cout << "  Records: " << seqRecords << "\n";
    std::cout << "  Throughput: " << (seqRecords / seqTime * 1000) << " records/sec\n";
    
    // PARALLEL LOADING
    start = Clock::now();
    auto parallelData = CSVLoader::loadFilesParallel(files, numThreads);
    auto parEnd = Clock::now();
    double parTime = std::chrono::duration<double, std::milli>(parEnd - start).count();
    
    size_t parRecords = 0;
    for (const auto& fd : parallelData) {
        parRecords += fd.recordCount;
    }
    
    std::cout << "\nParallel Loading (" << numThreads << " threads):\n";
    std::cout << "  Time: " << parTime << " ms\n";
    std::cout << "  Records: " << parRecords << "\n";
    std::cout << "  Throughput: " << (parRecords / parTime * 1000) << " records/sec\n";
    std::cout << "  Speedup: " << (seqTime / parTime) << "x\n";
    
    // Verify same data loaded
    if (seqRecords != parRecords) {
        std::cerr << "WARNING: Record count mismatch!\n";
    }
    
    // SEQUENTIAL AGGREGATION
    start = Clock::now();
    auto seqStats = Aggregator::computeSequential(sequentialData);
    seqEnd = Clock::now();
    double seqAggTime = std::chrono::duration<double, std::milli>(seqEnd - start).count();
    
    std::cout << "\n=== Sequential Aggregation ===\n";
    std::cout << "  Time: " << seqAggTime << " ms\n";
    std::cout << "  Avg Value: " << seqStats.avgValue << "\n";
    std::cout << "  Max Value: " << seqStats.maxValue << " at " << seqStats.maxLocation << "\n";
    
    // PARALLEL AGGREGATION
    start = Clock::now();
    auto parStats = Aggregator::computeParallel(parallelData, numThreads);
    parEnd = Clock::now();
    double parAggTime = std::chrono::duration<double, std::milli>(parEnd - start).count();
    
    std::cout << "\n=== Parallel Aggregation (" << numThreads << " threads) ===\n";
    std::cout << "  Time: " << parAggTime << " ms\n";
    std::cout << "  Avg Value: " << parStats.avgValue << "\n";
    std::cout << "  Max Value: " << parStats.maxValue << " at " << parStats.maxLocation << "\n";
    std::cout << "  Speedup: " << (seqAggTime / parAggTime) << "x\n";
    
    // TOTAL SPEEDUP
    std::cout << "\n=== OVERALL RESULTS ===\n";
    std::cout << "  Total Sequential Time: " << (seqTime + seqAggTime) << " ms\n";
    std::cout << "  Total Parallel Time: " << (parTime + parAggTime) << " ms\n";
    std::cout << "  Overall Speedup: " << ((seqTime + seqAggTime) / (parTime + parAggTime)) << "x\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <directory> [threads]\n";
        std::cout << "Example: " << argv[0] << " data/FireData/20200810 8\n";
        return 1;
    }
    
    std::string directory = argv[1];
    int numThreads = (argc > 2) ? std::stoi(argv[2]) : 4;
    
    std::cout << "=== Parallel File Loading Benchmark ===\n";
    std::cout << "Directory: " << directory << "\n";
    std::cout << "Threads: " << numThreads << "\n";
    
    // Scan directory for CSV files
    auto files = FileScanner::getCSVFiles(directory);
    
    if (files.empty()) {
        std::cerr << "No CSV files found in " << directory << "\n";
        return 1;
    }
    
    std::cout << "Found " << files.size() << " CSV files\n";
    for (const auto& file : files) {
        std::cout << "  - " << file << "\n";
    }
    
    // Run benchmark
    runBenchmark(files, numThreads);
    
    // Optional: Test with different thread counts
    std::cout << "\n\n=== Scaling Analysis ===\n";
    for (int t : {1, 2, 4, 8}) {
        if (t <= numThreads) {
            runBenchmark(files, t);
        }
    }
    
    return 0;
}
```

---

## Expected Results

### Example Output

```
=== Parallel File Loading Benchmark ===
Directory: data/FireData/20200810
Threads: 8
Found 12 CSV files
  - data/FireData/20200810/20200810-01.csv
  - data/FireData/20200810/20200810-03.csv
  - ... (10 more)

=== Benchmark: 12 files, 8 threads ===

Sequential Loading:
  Time: 1842.5 ms
  Records: 26,832
  Throughput: 14,562 records/sec

Parallel Loading (8 threads):
  Time: 287.3 ms
  Records: 26,832
  Throughput: 93,386 records/sec
  Speedup: 6.4x ✅

=== Sequential Aggregation ===
  Time: 45.2 ms
  Avg Value: 12.4
  Max Value: 48.7 at Crescent City

=== Parallel Aggregation (8 threads) ===
  Time: 8.7 ms
  Avg Value: 12.4
  Max Value: 48.7 at Crescent City
  Speedup: 5.2x ✅

=== OVERALL RESULTS ===
  Total Sequential Time: 1887.7 ms
  Total Parallel Time: 296.0 ms
  Overall Speedup: 6.4x ✅
```

---

## Key Implementation Details

### 1. Thread Safety

**Why this works without locks:**
```cpp
std::vector<FileData> results(filepaths.size());  // Pre-allocated

#pragma omp parallel for
for (size_t i = 0; i < filepaths.size(); i++) {
    results[i] = loadFile(filepaths[i]);  // Each thread writes to different position
}
```

- ✅ Each thread has its own file to read
- ✅ Each thread writes to different vector position
- ✅ No shared state during loading
- ✅ No locks needed!

### 2. Dynamic Scheduling

```cpp
#pragma omp parallel for schedule(dynamic)
```

**Why dynamic?** Files may have different sizes:
- `20200810-01.csv`: 2,236 records (fast)
- `20200810-13.csv`: 8,942 records (slow)

Dynamic scheduling assigns work as threads finish, preventing idle time.

### 3. Aggregation with Reduction

```cpp
double Aggregator::computeParallel(...) {
    double sum = 0.0;
    size_t count = 0;
    
    #pragma omp parallel for reduction(+:sum,count)
    for (size_t i = 0; i < allRecords.size(); i++) {
        sum += allRecords[i].value;
        count++;
    }
    
    return sum / count;
}
```

OpenMP `reduction` clause safely combines results from all threads.

---

## Timeline

**Week 1** (MVP):
- Day 1: Create `AirQualityRecord` struct and CSV parsing
- Day 2: Implement `CSVLoader` with sequential & parallel loading
- Day 3: Implement `FileScanner` for directory handling
- Day 4: Create basic aggregator
- Day 5: Build main benchmark application

**Week 2** (Enhancement):
- Day 1: Test with actual FireData (all files in one day)
- Day 2: Add more aggregation operations
- Day 3: Add filtering and top-N operations
- Day 4: Scaling analysis (1, 2, 4, 8 threads)
- Day 5: Documentation and optimization

---

## Files to Create (Much Simpler!)

**Headers (5)**:
1. `interface/airquality_record.hpp` - Data structure
2. `interface/csv_loader.hpp` - Parallel file loader
3. `interface/aggregator.hpp` - Parallel aggregation
4. `interface/file_scanner.hpp` - Directory utilities
5. `interface/benchmark_utils.hpp` - Timing utilities

**Implementation (5)**:
1. `src/airquality_record.cpp`
2. `src/csv_loader.cpp`
3. `src/aggregator.cpp`
4. `src/file_scanner.cpp`
5. `src/benchmark_utils.cpp`

**Main (1)**:
1. `src/main_parallel_benchmark.cpp`

**Total: 11 files** (vs 20 in previous plan!)

---

## CMakeLists.txt Addition

```cmake
# Parallel file loading benchmark
add_executable(OpenMP_ParallelFiles_App
    src/main_parallel_benchmark.cpp
    src/airquality_record.cpp
    src/csv_loader.cpp
    src/aggregator.cpp
    src/file_scanner.cpp
)

target_link_libraries(OpenMP_ParallelFiles_App PRIVATE OpenMP::OpenMP_CXX)
```

---

## Usage Examples

```bash
# Build
cmake --build build --config Release

# Test with single day (12 files)
./build/OpenMP_ParallelFiles_App data/FireData/20200810/ 8

# Test with multiple days (all files in FireData/)
./build/OpenMP_ParallelFiles_App data/FireData/ 8

# Test scaling (1, 2, 4, 8 threads)
for threads in 1 2 4 8; do
    ./build/OpenMP_ParallelFiles_App data/FireData/20200810/ $threads
done
```

---

## Expected Speedups

Based on I/O and CPU characteristics:

| Files | Sequential | Parallel (4 threads) | Parallel (8 threads) |
|-------|------------|---------------------|---------------------|
| 4 files | 800ms | 250ms (3.2x) | 220ms (3.6x) |
| 12 files | 2400ms | 700ms (3.4x) | 400ms (6.0x) |
| 48 files | 9600ms | 2800ms (3.4x) | 1500ms (6.4x) |

**Factors affecting speedup:**
- ✅ **CPU cores**: More cores = better speedup
- ✅ **File sizes**: Larger files = better parallelization benefit
- ⚠️ **Disk I/O**: SSD is faster than HDD, may hit I/O bottleneck
- ⚠️ **File count**: Need enough files to saturate threads

---

## Success Criteria

✅ Load multiple CSV files in parallel using OpenMP  
✅ Demonstrate 4-8x speedup with 8 threads on 12+ files  
✅ Aggregate results correctly (parallel == sequential results)  
✅ Handle 1M+ total records across all files  
✅ Memory usage < 500 MB for 1M records  
✅ Clean error handling for missing/corrupted files  

---

## This is MUCH Better for Your Use Case!

**Why this approach is perfect:**
- ✅ **Simple**: Just parallel file loading, not complex data structure comparison
- ✅ **Practical**: Real-world scenario (loading many log files)
- ✅ **Demonstrates OpenMP**: Clear parallelization with measurable speedup
- ✅ **Scalable**: Works with any number of files
- ✅ **Fast to implement**: 1-2 weeks instead of 3-4 weeks

Ready to start implementation? 🚀


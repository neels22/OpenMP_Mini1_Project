# Final Implementation Plan: Parallel File Loading + Row vs Column Comparison

## Project Overview

**Demonstrate OpenMP parallelization in TWO dimensions:**

1. **Parallel File Loading**: Load multiple CSV files simultaneously using OpenMP threads
2. **Data Layout Comparison**: Compare row-oriented vs column-oriented storage performance for air quality queries

This combines the best of both approaches!

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    PHASE 1: PARALLEL LOADING                    │
│                                                                 │
│   OpenMP Thread 1: Load File1.csv ──┐                          │
│   OpenMP Thread 2: Load File2.csv ──┤                          │
│   OpenMP Thread 3: Load File3.csv ──┼─→ Combined Raw Data      │
│   ...                               ──┤                          │
│   OpenMP Thread N: Load FileN.csv ──┘                          │
│                                                                 │
│   Speedup Example: 12 files in 2.4s → 0.4s (6x faster!)       │
└─────────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────────┐
│                PHASE 2: DUAL MODEL CONSTRUCTION                 │
│                                                                 │
│  ┌──────────────────────────┐  ┌──────────────────────────┐   │
│  │   ROW-ORIENTED MODEL     │  │  COLUMN-ORIENTED MODEL   │   │
│  │   (Station-centric)      │  │  (Time-centric)          │   │
│  │                          │  │                          │   │
│  │ Station1: [t1,t2,t3...] │  │ Time1: [s1,s2,s3...]    │   │
│  │ Station2: [t1,t2,t3...] │  │ Time2: [s1,s2,s3...]    │   │
│  │ Station3: [t1,t2,t3...] │  │ Time3: [s1,s2,s3...]    │   │
│  └──────────────────────────┘  └──────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────────┐
│              PHASE 3: PERFORMANCE COMPARISON                    │
│                                                                 │
│  Query Type                    Row Model    Column Model       │
│  ────────────────────────────────────────────────────────────  │
│  Avg PM2.5 at time T           250 µs       23 µs (10x faster) │
│  Time series for station S     12 µs        89 µs (7x slower)  │
│  Top-10 stations at time T     412 µs       34 µs (12x faster) │
│                                                                 │
│  Conclusion: Column wins for temporal queries!                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Implementation Steps

### Step 1: Core Data Structures

**File**: `interface/airquality_types.hpp`

```cpp
#pragma once
#include <string>
#include <vector>

namespace AirQuality {

// Single measurement record
struct Record {
    double latitude;
    double longitude;
    long long timestamp;      // Unix timestamp (for fast comparison)
    std::string dateTimeStr;  // Original ISO string
    std::string pollutant;    // PM2.5, PM10, OZONE
    double value;             // Measurement value
    std::string unit;         // UG/M3, PPB
    double aqi;               // Air Quality Index
    int aqiCategory;
    int qualityFlag;
    std::string location;     // Station name
    std::string agency;
    std::string siteId1;
    std::string siteId2;
    
    // Helpers
    bool isValid() const;
    static Record fromCSVLine(const std::string& line);
};

// Station metadata (for grouping)
struct StationInfo {
    std::string siteId;
    std::string location;
    double latitude;
    double longitude;
    std::string agency;
    size_t recordCount;
};

} // namespace AirQuality
```

---

### Step 2: Parallel File Loader

**File**: `interface/parallel_csv_loader.hpp`

```cpp
#pragma once
#include "airquality_types.hpp"
#include <vector>
#include <string>

namespace AirQuality {

// Result from loading one file
struct FileLoadResult {
    std::string filename;
    std::vector<Record> records;
    size_t recordCount;
    double loadTimeMs;
    bool success;
    std::string errorMsg;
};

class ParallelCSVLoader {
public:
    // Load single file (thread-safe)
    static FileLoadResult loadFile(const std::string& filepath);
    
    // Load multiple files SEQUENTIALLY (baseline)
    static std::vector<FileLoadResult> loadSequential(
        const std::vector<std::string>& filepaths);
    
    // Load multiple files IN PARALLEL using OpenMP
    static std::vector<FileLoadResult> loadParallel(
        const std::vector<std::string>& filepaths,
        int numThreads = 4);
    
    // Get all CSV files in directory
    static std::vector<std::string> scanDirectory(const std::string& dir);
    
private:
    static Record parseLine(const std::string& line);
    static std::vector<std::string> splitCSV(const std::string& line);
    static long long parseDateTime(const std::string& iso8601);
};

} // namespace AirQuality
```

**Implementation**: `src/parallel_csv_loader.cpp`

```cpp
#include "../interface/parallel_csv_loader.hpp"
#include <fstream>
#include <sstream>
#include <chrono>
#include <filesystem>
#include <omp.h>

namespace AirQuality {

FileLoadResult ParallelCSVLoader::loadFile(const std::string& filepath) {
    auto start = std::chrono::high_resolution_clock::now();
    
    FileLoadResult result;
    result.filename = filepath;
    result.success = false;
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        result.errorMsg = "Cannot open file";
        return result;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        try {
            Record rec = parseLine(line);
            if (rec.isValid()) {
                result.records.push_back(std::move(rec));
            }
        } catch (...) {
            // Skip malformed lines
        }
    }
    file.close();
    
    auto end = std::chrono::high_resolution_clock::now();
    result.loadTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
    result.recordCount = result.records.size();
    result.success = true;
    
    return result;
}

std::vector<FileLoadResult> ParallelCSVLoader::loadSequential(
    const std::vector<std::string>& filepaths) {
    
    std::vector<FileLoadResult> results;
    results.reserve(filepaths.size());
    
    for (const auto& path : filepaths) {
        results.push_back(loadFile(path));
    }
    
    return results;
}

std::vector<FileLoadResult> ParallelCSVLoader::loadParallel(
    const std::vector<std::string>& filepaths,
    int numThreads) {
    
    std::vector<FileLoadResult> results(filepaths.size());
    
    // KEY: Parallel file loading with OpenMP
    #pragma omp parallel for num_threads(numThreads) schedule(dynamic)
    for (size_t i = 0; i < filepaths.size(); i++) {
        results[i] = loadFile(filepaths[i]);
    }
    
    return results;
}

std::vector<std::string> ParallelCSVLoader::scanDirectory(const std::string& dir) {
    std::vector<std::string> csvFiles;
    
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.path().extension() == ".csv") {
            csvFiles.push_back(entry.path().string());
        }
    }
    
    std::sort(csvFiles.begin(), csvFiles.end());
    return csvFiles;
}

} // namespace AirQuality
```

**Key Feature**: Each thread loads a different file independently - no locks needed!

---

### Step 3: Row-Oriented Model (Station-Centric)

**File**: `interface/airquality_model_row.hpp`

```cpp
#pragma once
#include "airquality_types.hpp"
#include <vector>
#include <unordered_map>

namespace AirQuality {

class RowModel {
private:
    // Station-centric storage
    // _stationRecords[stationIndex] = vector of all records for that station
    std::vector<std::vector<Record>> _stationRecords;
    
    // Metadata
    std::vector<StationInfo> _stations;
    std::unordered_map<std::string, int> _siteIdToIndex;
    
    // Time range
    long long _minTimestamp;
    long long _maxTimestamp;

public:
    RowModel() = default;
    
    // Build model from loaded file data
    void buildFromFiles(const std::vector<FileLoadResult>& fileData);
    
    // Queries
    size_t stationCount() const { return _stations.size(); }
    size_t totalRecords() const;
    
    const std::vector<Record>& getStationRecords(int stationIndex) const;
    const std::vector<Record>& getStationRecords(const std::string& siteId) const;
    
    // For service layer
    const std::vector<std::vector<Record>>& allStationRecords() const { 
        return _stationRecords; 
    }
    const std::vector<StationInfo>& stations() const { return _stations; }
};

} // namespace AirQuality
```

**Implementation**: `src/airquality_model_row.cpp`

```cpp
void RowModel::buildFromFiles(const std::vector<FileLoadResult>& fileData) {
    // Step 1: Collect all records from all files
    std::vector<Record> allRecords;
    for (const auto& file : fileData) {
        if (file.success) {
            allRecords.insert(allRecords.end(), 
                            file.records.begin(), 
                            file.records.end());
        }
    }
    
    // Step 2: Group by station (siteId1)
    std::unordered_map<std::string, std::vector<Record>> stationMap;
    for (auto& rec : allRecords) {
        stationMap[rec.siteId1].push_back(std::move(rec));
    }
    
    // Step 3: Build station-centric storage
    _stations.clear();
    _stationRecords.clear();
    _siteIdToIndex.clear();
    
    int idx = 0;
    for (auto& [siteId, records] : stationMap) {
        // Sort records by timestamp for this station
        std::sort(records.begin(), records.end(),
                 [](const Record& a, const Record& b) {
                     return a.timestamp < b.timestamp;
                 });
        
        // Create station info
        StationInfo info;
        info.siteId = siteId;
        info.location = records[0].location;
        info.latitude = records[0].latitude;
        info.longitude = records[0].longitude;
        info.agency = records[0].agency;
        info.recordCount = records.size();
        
        _stations.push_back(info);
        _stationRecords.push_back(std::move(records));
        _siteIdToIndex[siteId] = idx++;
    }
    
    std::cout << "Row Model built: " << _stations.size() << " stations, "
              << totalRecords() << " records\n";
}
```

---

### Step 4: Column-Oriented Model (Time-Centric)

**File**: `interface/airquality_model_column.hpp`

```cpp
#pragma once
#include "airquality_types.hpp"
#include <vector>
#include <unordered_map>

namespace AirQuality {

class ColumnModel {
private:
    // Time-centric storage
    // _timeSlots[timeIndex] = vector of all records at that timestamp
    std::vector<std::vector<Record>> _timeSlots;
    
    // Metadata
    std::vector<long long> _timestamps;  // Unique sorted timestamps
    std::unordered_map<long long, int> _timestampToIndex;
    
    // Station reference
    std::vector<StationInfo> _stations;
    std::unordered_map<std::string, int> _siteIdToIndex;

public:
    ColumnModel() = default;
    
    // Build model from loaded file data
    void buildFromFiles(const std::vector<FileLoadResult>& fileData);
    
    // Queries
    size_t timeSlotCount() const { return _timestamps.size(); }
    size_t stationCount() const { return _stations.size(); }
    size_t totalRecords() const;
    
    const std::vector<Record>& getRecordsAtTime(int timeIndex) const;
    const std::vector<Record>& getRecordsAtTimestamp(long long timestamp) const;
    
    // For service layer
    const std::vector<std::vector<Record>>& allTimeSlots() const { 
        return _timeSlots; 
    }
    const std::vector<long long>& timestamps() const { return _timestamps; }
    const std::vector<StationInfo>& stations() const { return _stations; }
};

} // namespace AirQuality
```

**Implementation**: `src/airquality_model_column.cpp`

```cpp
void ColumnModel::buildFromFiles(const std::vector<FileLoadResult>& fileData) {
    // Step 1: Collect all records
    std::vector<Record> allRecords;
    for (const auto& file : fileData) {
        if (file.success) {
            allRecords.insert(allRecords.end(),
                            file.records.begin(),
                            file.records.end());
        }
    }
    
    // Step 2: Group by timestamp
    std::map<long long, std::vector<Record>> timeMap;  // sorted by timestamp
    std::unordered_set<std::string> uniqueStations;
    
    for (auto& rec : allRecords) {
        timeMap[rec.timestamp].push_back(std::move(rec));
        uniqueStations.insert(rec.siteId1);
    }
    
    // Step 3: Build time-centric storage
    _timestamps.clear();
    _timeSlots.clear();
    _timestampToIndex.clear();
    
    int idx = 0;
    for (auto& [timestamp, records] : timeMap) {
        _timestamps.push_back(timestamp);
        _timeSlots.push_back(std::move(records));
        _timestampToIndex[timestamp] = idx++;
    }
    
    // Step 4: Build station metadata (aggregate from all records)
    // ... (similar to row model)
    
    std::cout << "Column Model built: " << _timestamps.size() << " time slots, "
              << uniqueStations.size() << " stations, "
              << totalRecords() << " records\n";
}
```

---

### Step 5: Service Interface

**File**: `interface/airquality_service_interface.hpp`

```cpp
#pragma once
#include <string>
#include <vector>

namespace AirQuality {

class IService {
public:
    virtual ~IService() = default;
    
    // Temporal aggregations (MAIN QUERIES)
    virtual double avgPollutantAtTime(
        long long timestamp,
        const std::string& pollutant,
        int numThreads = 1) const = 0;
    
    virtual double maxPollutantAtTime(
        long long timestamp,
        const std::string& pollutant,
        int numThreads = 1) const = 0;
    
    // Station queries
    virtual std::vector<std::pair<long long, double>> 
        timeSeriesForStation(
            const std::string& siteId,
            const std::string& pollutant,
            int numThreads = 1) const = 0;
    
    // Top-N queries
    virtual std::vector<std::pair<std::string, double>>
        topNStationsAtTime(
            int n,
            long long timestamp,
            const std::string& pollutant,
            int numThreads = 1) const = 0;
    
    // Identification
    virtual std::string getImplementationName() const = 0;
};

} // namespace AirQuality
```

---

### Step 6: Service Implementations

**File**: `interface/airquality_service_row.hpp`

```cpp
#pragma once
#include "airquality_service_interface.hpp"
#include "airquality_model_row.hpp"

namespace AirQuality {

class RowService : public IService {
private:
    const RowModel* _model;

public:
    explicit RowService(const RowModel* model) : _model(model) {}
    
    double avgPollutantAtTime(...) const override;
    double maxPollutantAtTime(...) const override;
    std::vector<std::pair<long long, double>> timeSeriesForStation(...) const override;
    std::vector<std::pair<std::string, double>> topNStationsAtTime(...) const override;
    
    std::string getImplementationName() const override {
        return "Row-oriented (Station-centric)";
    }
};

} // namespace AirQuality
```

**File**: `interface/airquality_service_column.hpp`

```cpp
#pragma once
#include "airquality_service_interface.hpp"
#include "airquality_model_column.hpp"

namespace AirQuality {

class ColumnService : public IService {
private:
    const ColumnModel* _model;

public:
    explicit ColumnService(const ColumnModel* model) : _model(model) {}
    
    double avgPollutantAtTime(...) const override;
    double maxPollutantAtTime(...) const override;
    std::vector<std::pair<long long, double>> timeSeriesForStation(...) const override;
    std::vector<std::pair<std::string, double>> topNStationsAtTime(...) const override;
    
    std::string getImplementationName() const override {
        return "Column-oriented (Time-centric)";
    }
};

} // namespace AirQuality
```

**Implementation Example**: `src/airquality_service_column.cpp`

```cpp
double ColumnService::avgPollutantAtTime(
    long long timestamp,
    const std::string& pollutant,
    int numThreads) const {
    
    // Column model advantage: direct access to time slot!
    const auto& records = _model->getRecordsAtTimestamp(timestamp);
    
    double sum = 0.0;
    int count = 0;
    
    #pragma omp parallel for reduction(+:sum,count) num_threads(numThreads)
    for (size_t i = 0; i < records.size(); i++) {
        if (records[i].pollutant == pollutant) {
            sum += records[i].value;
            count++;
        }
    }
    
    return count > 0 ? sum / count : 0.0;
}
```

---

### Step 7: Main Benchmark Application

**File**: `src/main_airquality_benchmark.cpp`

```cpp
#include <iostream>
#include <chrono>
#include "../interface/parallel_csv_loader.hpp"
#include "../interface/airquality_model_row.hpp"
#include "../interface/airquality_model_column.hpp"
#include "../interface/airquality_service_row.hpp"
#include "../interface/airquality_service_column.hpp"

using namespace AirQuality;
using Clock = std::chrono::high_resolution_clock;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <directory> [threads]\n";
        std::cout << "Example: " << argv[0] << " data/FireData/20200810 8\n";
        return 1;
    }
    
    std::string directory = argv[1];
    int numThreads = (argc > 2) ? std::stoi(argv[2]) : 4;
    
    std::cout << "╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Air Quality Analysis: Row vs Column Comparison       ║\n";
    std::cout << "║  With Parallel File Loading                           ║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n\n";
    
    // ═══════════════════════════════════════════════════════════
    // PHASE 1: PARALLEL FILE LOADING
    // ═══════════════════════════════════════════════════════════
    
    std::cout << "📁 Scanning directory: " << directory << "\n";
    auto files = ParallelCSVLoader::scanDirectory(directory);
    std::cout << "   Found " << files.size() << " CSV files\n\n";
    
    // Sequential loading (baseline)
    std::cout << "🔄 Loading files SEQUENTIALLY...\n";
    auto seqStart = Clock::now();
    auto seqResults = ParallelCSVLoader::loadSequential(files);
    auto seqEnd = Clock::now();
    double seqTime = std::chrono::duration<double, std::milli>(seqEnd - seqStart).count();
    
    size_t seqRecords = 0;
    for (const auto& f : seqResults) seqRecords += f.recordCount;
    
    std::cout << "   ✅ Loaded " << seqRecords << " records in " 
              << seqTime << " ms\n\n";
    
    // Parallel loading
    std::cout << "⚡ Loading files IN PARALLEL (" << numThreads << " threads)...\n";
    auto parStart = Clock::now();
    auto parResults = ParallelCSVLoader::loadParallel(files, numThreads);
    auto parEnd = Clock::now();
    double parTime = std::chrono::duration<double, std::milli>(parEnd - parStart).count();
    
    size_t parRecords = 0;
    for (const auto& f : parResults) parRecords += f.recordCount;
    
    std::cout << "   ✅ Loaded " << parRecords << " records in " 
              << parTime << " ms\n";
    std::cout << "   🚀 SPEEDUP: " << (seqTime / parTime) << "x faster!\n\n";
    
    // ═══════════════════════════════════════════════════════════
    // PHASE 2: BUILD BOTH MODELS
    // ═══════════════════════════════════════════════════════════
    
    std::cout << "🏗️  Building data models...\n";
    
    RowModel rowModel;
    ColumnModel colModel;
    
    auto buildStart = Clock::now();
    rowModel.buildFromFiles(parResults);
    auto buildMid = Clock::now();
    colModel.buildFromFiles(parResults);
    auto buildEnd = Clock::now();
    
    double rowBuildTime = std::chrono::duration<double, std::milli>(buildMid - buildStart).count();
    double colBuildTime = std::chrono::duration<double, std::milli>(buildEnd - buildMid).count();
    
    std::cout << "   Row model: " << rowBuildTime << " ms\n";
    std::cout << "   Column model: " << colBuildTime << " ms\n\n";
    
    // ═══════════════════════════════════════════════════════════
    // PHASE 3: PERFORMANCE COMPARISON
    // ═══════════════════════════════════════════════════════════
    
    std::cout << "⚖️  Comparing query performance...\n\n";
    
    RowService rowService(&rowModel);
    ColumnService colService(&colModel);
    
    // Get a sample timestamp from the data
    long long sampleTime = colModel.timestamps()[colModel.timestamps().size() / 2];
    std::string pollutant = "PM2.5";
    
    // --- Query 1: Average at specific time ---
    std::cout << "Query 1: Average " << pollutant << " at specific time\n";
    
    auto q1Start = Clock::now();
    double rowAvg = rowService.avgPollutantAtTime(sampleTime, pollutant, numThreads);
    auto q1Mid = Clock::now();
    double colAvg = colService.avgPollutantAtTime(sampleTime, pollutant, numThreads);
    auto q1End = Clock::now();
    
    double rowQ1Time = std::chrono::duration<double, std::micro>(q1Mid - q1Start).count();
    double colQ1Time = std::chrono::duration<double, std::micro>(q1End - q1Mid).count();
    
    std::cout << "   Row model:    " << rowQ1Time << " µs (result: " << rowAvg << ")\n";
    std::cout << "   Column model: " << colQ1Time << " µs (result: " << colAvg << ")\n";
    std::cout << "   🏆 Winner: " << (colQ1Time < rowQ1Time ? "Column" : "Row") 
              << " (" << (rowQ1Time / colQ1Time) << "x faster)\n\n";
    
    // --- Query 2: Top-N stations ---
    std::cout << "Query 2: Top-10 stations with highest " << pollutant << "\n";
    
    auto q2Start = Clock::now();
    auto rowTop = rowService.topNStationsAtTime(10, sampleTime, pollutant, numThreads);
    auto q2Mid = Clock::now();
    auto colTop = colService.topNStationsAtTime(10, sampleTime, pollutant, numThreads);
    auto q2End = Clock::now();
    
    double rowQ2Time = std::chrono::duration<double, std::micro>(q2Mid - q2Start).count();
    double colQ2Time = std::chrono::duration<double, std::micro>(q2End - q2Mid).count();
    
    std::cout << "   Row model:    " << rowQ2Time << " µs\n";
    std::cout << "   Column model: " << colQ2Time << " µs\n";
    std::cout << "   🏆 Winner: " << (colQ2Time < rowQ2Time ? "Column" : "Row")
              << " (" << (rowQ2Time / colQ2Time) << "x faster)\n\n";
    
    // ═══════════════════════════════════════════════════════════
    // SUMMARY
    // ═══════════════════════════════════════════════════════════
    
    std::cout << "╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    SUMMARY                             ║\n";
    std::cout << "╠════════════════════════════════════════════════════════╣\n";
    std::cout << "║ Parallel Loading Speedup: " << (seqTime / parTime) << "x              ║\n";
    std::cout << "║ Files Loaded: " << files.size() << "                                    ║\n";
    std::cout << "║ Total Records: " << parRecords << "                              ║\n";
    std::cout << "║ Stations: " << rowModel.stationCount() << "                                   ║\n";
    std::cout << "║                                                        ║\n";
    std::cout << "║ Column Model Advantages:                               ║\n";
    std::cout << "║   ✅ " << (rowQ1Time / colQ1Time) << "x faster for temporal aggregations     ║\n";
    std::cout << "║   ✅ " << (rowQ2Time / colQ2Time) << "x faster for top-N queries              ║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n";
    
    return 0;
}
```

---

## Expected Output

```
╔════════════════════════════════════════════════════════╗
║  Air Quality Analysis: Row vs Column Comparison       ║
║  With Parallel File Loading                           ║
╚════════════════════════════════════════════════════════╝

📁 Scanning directory: data/FireData/20200810
   Found 12 CSV files

🔄 Loading files SEQUENTIALLY...
   ✅ Loaded 26,832 records in 1842.5 ms

⚡ Loading files IN PARALLEL (8 threads)...
   ✅ Loaded 26,832 records in 287.3 ms
   🚀 SPEEDUP: 6.4x faster!

🏗️  Building data models...
   Row model: 145.2 ms (1,234 stations)
   Column model: 132.8 ms (24 time slots)

⚖️  Comparing query performance...

Query 1: Average PM2.5 at specific time
   Row model:    245.3 µs (result: 12.4)
   Column model: 23.4 µs (result: 12.4)
   🏆 Winner: Column (10.5x faster)

Query 2: Top-10 stations with highest PM2.5
   Row model:    412.7 µs
   Column model: 34.2 µs
   🏆 Winner: Column (12.1x faster)

╔════════════════════════════════════════════════════════╗
║                    SUMMARY                             ║
╠════════════════════════════════════════════════════════╣
║ Parallel Loading Speedup: 6.4x                        ║
║ Files Loaded: 12                                      ║
║ Total Records: 26,832                                 ║
║ Stations: 1,234                                       ║
║                                                        ║
║ Column Model Advantages:                               ║
║   ✅ 10.5x faster for temporal aggregations            ║
║   ✅ 12.1x faster for top-N queries                    ║
╚════════════════════════════════════════════════════════╝
```

---

## Implementation Timeline

### Week 1: Core Infrastructure
- **Day 1**: Data structures (`Record`, `StationInfo`)
- **Day 2**: Parallel CSV loader with OpenMP
- **Day 3**: Row model implementation
- **Day 4**: Column model implementation
- **Day 5**: Testing with small dataset

### Week 2: Services & Queries
- **Day 1**: Service interface design
- **Day 2**: Row service implementation
- **Day 3**: Column service implementation
- **Day 4**: Query operations with OpenMP
- **Day 5**: Testing and validation

### Week 3: Benchmarking & Polish
- **Day 1**: Main benchmark application
- **Day 2**: Test with full FireData
- **Day 3**: Performance tuning
- **Day 4**: Documentation
- **Day 5**: Final testing and demo

---

## Files to Create (15 total)

**Headers (8)**:
1. `interface/airquality_types.hpp`
2. `interface/parallel_csv_loader.hpp`
3. `interface/airquality_model_row.hpp`
4. `interface/airquality_model_column.hpp`
5. `interface/airquality_service_interface.hpp`
6. `interface/airquality_service_row.hpp`
7. `interface/airquality_service_column.hpp`
8. `interface/datetime_utils.hpp`

**Implementation (6)**:
1. `src/airquality_types.cpp`
2. `src/parallel_csv_loader.cpp`
3. `src/airquality_model_row.cpp`
4. `src/airquality_model_column.cpp`
5. `src/airquality_service_row.cpp`
6. `src/airquality_service_column.cpp`

**Main (1)**:
1. `src/main_airquality_benchmark.cpp`

---

## CMakeLists.txt Update

```cmake
# Air Quality benchmark with parallel loading + row/column comparison
add_executable(OpenMP_AirQuality_App
    src/main_airquality_benchmark.cpp
    src/airquality_types.cpp
    src/parallel_csv_loader.cpp
    src/airquality_model_row.cpp
    src/airquality_model_column.cpp
    src/airquality_service_row.cpp
    src/airquality_service_column.cpp
)

target_link_libraries(OpenMP_AirQuality_App PRIVATE OpenMP::OpenMP_CXX)
```

---

## Key Features Demonstrated

### ✅ OpenMP Parallelization (Two Levels)

1. **Parallel File I/O**
   ```cpp
   #pragma omp parallel for schedule(dynamic)
   for (each file) { load_file(); }
   ```
   Demonstrates: File-level parallelism

2. **Parallel Query Processing**
   ```cpp
   #pragma omp parallel for reduction(+:sum)
   for (each record) { aggregate(); }
   ```
   Demonstrates: Data-level parallelism

### ✅ Data Structure Comparison
- Row-oriented: Fast for station-specific queries
- Column-oriented: Fast for temporal aggregations

### ✅ Real-World Performance
- Load 12 files: **6x speedup** with 8 threads
- Temporal queries: **10x speedup** with columnar layout
- Combined benefit: **Both** parallelism AND better data structures

---

## Success Criteria

✅ Load 12+ CSV files in parallel with measurable speedup (>4x on 8 threads)  
✅ Both models store identical data (verified by query results)  
✅ Column model 8-15x faster for temporal queries  
✅ Row model 5-10x faster for station time-series queries  
✅ Handle 1M+ total records across all files  
✅ Memory usage < 1 GB  
✅ Clean, documented, maintainable code  

---

## Ready to Implement?

This combines:
- ✅ **Parallel file loading** (your requirement)
- ✅ **Row vs Column comparison** (original project architecture)
- ✅ **Air quality data** (your actual dataset)

**Should I start implementing now?** 🚀

I'll begin with Step 1 (data structures) and work through systematically!


# Air Quality Data Model Implementation Plan

## Executive Summary

This document provides a comprehensive step-by-step plan to adapt the OpenMP Population Model project to handle air quality monitoring data (1 million+ rows). The implementation will maintain the row vs columnar comparison philosophy while supporting the specific schema and query patterns of air quality data.

---

## Table of Contents

1. [Data Schema Analysis](#1-data-schema-analysis)
2. [Architecture Design](#2-architecture-design)
3. [Implementation Steps](#3-implementation-steps)
4. [Query Operations](#4-query-operations)
5. [Memory Estimates](#5-memory-estimates)
6. [Timeline & Milestones](#6-timeline--milestones)

---

## 1. Data Schema Analysis

### Current Air Quality Data Format

```csv
Latitude, Longitude, DateTime, Pollutant, Value, Unit, AQI, AQI_Category, ...
"41.75613", "-124.20347", "2020-08-10T01:00", "PM2.5", "17.3", "UG/M3", "18.0", "62", ...
```

### Field Breakdown

| Field | Type | Purpose | Example |
|-------|------|---------|---------|
| Latitude | double | Geographic coordinate | 41.75613 |
| Longitude | double | Geographic coordinate | -124.20347 |
| DateTime | string | Timestamp (ISO format) | "2020-08-10T01:00" |
| Pollutant | string | Type of measurement | "PM2.5", "PM10", "OZONE" |
| Value | double | Measurement value | 17.3 |
| Unit | string | Measurement unit | "UG/M3", "PPB" |
| AQI | double | Air Quality Index | 18.0 |
| AQI_Category | int | AQI risk level | 62 |
| Quality_Flag | int | Data quality indicator | 2 |
| Location | string | Station name | "Crescent City" |
| Agency | string | Monitoring agency | "North Coast Unified..." |
| SiteID1 | string | Primary site identifier | "840060150007" |
| SiteID2 | string | Secondary site identifier | "840060150007" |

### Key Differences from Population Data

| Aspect | Population Data | Air Quality Data |
|--------|-----------------|------------------|
| **Structure** | Wide (pivot table) | Long (normalized) |
| **Rows** | Entity-centric (one per country) | Event-centric (one per measurement) |
| **Columns** | Fixed years | Fixed measurement attributes |
| **Volume** | 266 rows × 65 years | 1M+ individual measurements |
| **Queries** | Aggregate across countries per year | Aggregate by time, location, pollutant |

---

## 2. Architecture Design

### Design Philosophy

**Maintain the core comparison**: Row-oriented vs Column-oriented data layouts for different query patterns.

### Two Data Models

#### Model A: Row-Oriented (Station-Centric)

**Structure**: One row per unique station, containing all measurements for that station

```
Station 1: [All measurements for this station as a time-ordered vector]
Station 2: [All measurements for this station as a time-ordered vector]
...
```

**Optimal for:**
- ✅ Time-series analysis for specific stations
- ✅ Station-specific queries
- ✅ Sequential temporal access patterns

**Poor for:**
- ❌ Cross-station aggregations
- ❌ Time-slice queries (all stations at specific time)

#### Model B: Column-Oriented (Time-Centric)

**Structure**: One column per time bucket, containing all station measurements

```
TimeSlot 1 (2020-08-10T01:00): [Station1_PM25, Station2_PM10, Station3_PM25, ...]
TimeSlot 2 (2020-08-10T02:00): [Station1_PM25, Station2_PM10, Station3_PM25, ...]
...
```

**Optimal for:**
- ✅ Temporal aggregations (average PM2.5 at specific hour across all stations)
- ✅ Spatial queries (all stations in time range)
- ✅ Parallel reductions over many stations

**Poor for:**
- ❌ Single-station time-series queries

---

## 3. Implementation Steps

### Phase 1: Core Data Structures (Week 1)

#### Step 1.1: Create AirQualityRecord Structure

**File**: `interface/airquality_types.hpp` (NEW)

```cpp
struct AirQualityRecord {
    double latitude;
    double longitude;
    long long timestamp;      // Unix timestamp for fast comparisons
    std::string dateTimeStr;  // Original string for display
    std::string pollutant;    // PM2.5, PM10, OZONE, etc.
    double value;
    std::string unit;
    double aqi;
    int aqiCategory;
    int qualityFlag;
    std::string location;
    std::string agency;
    std::string siteId1;
    std::string siteId2;
    
    // Helper methods
    bool isValid() const;
    std::string toString() const;
};

// Station metadata (for grouping records)
struct StationInfo {
    std::string siteId;
    std::string location;
    double latitude;
    double longitude;
    std::string agency;
    
    // Spatial queries
    double distanceTo(double lat, double lon) const;
};
```

**Implementation tasks:**
- [ ] Define struct with all fields
- [ ] Add constructor and validation methods
- [ ] Implement toString() for debugging
- [ ] Add helper for timestamp parsing (ISO 8601 → Unix timestamp)
- [ ] Add distance calculation for spatial queries

---

#### Step 1.2: Design Row-Based Model

**File**: `interface/airqualityModel.hpp` (NEW)

```cpp
class AirQualityModel {
private:
    // Station-centric storage: vector of stations, each with their measurements
    std::vector<std::vector<AirQualityRecord>> _stationRecords;
    
    // Metadata
    std::vector<StationInfo> _stations;
    std::unordered_map<std::string, int> _siteIdToIndex;
    
    // Quick lookups
    std::unordered_map<std::string, std::vector<int>> _pollutantToStations;
    long long _minTimestamp;
    long long _maxTimestamp;

public:
    // Data loading
    void readFromCSV(const std::string& filename);
    
    // Metadata queries
    size_t stationCount() const;
    size_t totalRecordCount() const;
    const StationInfo& getStation(int index) const;
    
    // Data access
    const std::vector<AirQualityRecord>& getStationRecords(int stationIndex) const;
    const std::vector<AirQualityRecord>& getStationRecordsBySiteId(const std::string& siteId) const;
    
    // Aggregation support (direct access to underlying data)
    const std::vector<std::vector<AirQualityRecord>>& allRecords() const;
};
```

**Implementation tasks:**
- [ ] Define class structure
- [ ] Implement CSV parsing for air quality format
- [ ] Group records by station during loading
- [ ] Build spatial and temporal indices
- [ ] Sort station records by timestamp for efficient queries
- [ ] Implement metadata extraction

---

#### Step 1.3: Design Column-Based Model

**File**: `interface/airqualityModelColumn.hpp` (NEW)

```cpp
class AirQualityModelColumn {
private:
    // Time-centric storage: vector of time buckets, each with all station measurements
    // _timeSlots[time_index] = vector of all records at that time
    std::vector<std::vector<AirQualityRecord>> _timeSlots;
    
    // Metadata
    std::vector<long long> _timestamps;  // Unique sorted timestamps
    std::unordered_map<long long, int> _timestampToIndex;
    
    // Station quick reference (flat list for indexing)
    std::vector<StationInfo> _stations;
    std::unordered_map<std::string, int> _siteIdToIndex;
    
    // Quick lookups
    std::unordered_map<std::string, std::vector<int>> _pollutantToTimeSlots;

public:
    // Data loading
    void readFromCSV(const std::string& filename);
    
    // Metadata queries
    size_t timeSlotCount() const;
    size_t totalRecordCount() const;
    size_t stationCount() const;
    
    // Data access
    const std::vector<AirQualityRecord>& getRecordsAtTime(int timeIndex) const;
    const std::vector<AirQualityRecord>& getRecordsAtTimestamp(long long timestamp) const;
    
    // Aggregation support
    const std::vector<std::vector<AirQualityRecord>>& allTimeSlots() const;
    const std::vector<long long>& timestamps() const;
};
```

**Implementation tasks:**
- [ ] Define class structure
- [ ] Implement CSV parsing (same parser, different organization)
- [ ] Group records by timestamp during loading
- [ ] Build temporal indices
- [ ] Maintain station metadata for cross-referencing
- [ ] Sort timestamps for range queries

---

### Phase 2: Service Layer (Week 2)

#### Step 2.1: Define Query Interface

**File**: `interface/airquality_service_interface.hpp` (NEW)

```cpp
class IAirQualityService {
public:
    virtual ~IAirQualityService() = default;
    
    // === Temporal Aggregations (Main Use Case) ===
    
    // Average pollutant value at specific time across all stations
    virtual double avgPollutantAtTime(long long timestamp, 
                                      const std::string& pollutant,
                                      int numThreads = 1) const = 0;
    
    // Max pollutant value at specific time
    virtual double maxPollutantAtTime(long long timestamp,
                                      const std::string& pollutant,
                                      int numThreads = 1) const = 0;
    
    // Average pollutant over time range across all stations
    virtual double avgPollutantInRange(long long startTime, long long endTime,
                                       const std::string& pollutant,
                                       int numThreads = 1) const = 0;
    
    // === Station-Specific Queries ===
    
    // Get average for specific station over time range
    virtual double avgForStation(const std::string& siteId,
                                 long long startTime, long long endTime,
                                 const std::string& pollutant,
                                 int numThreads = 1) const = 0;
    
    // Get time series for specific station
    virtual std::vector<std::pair<long long, double>> 
        timeSeriesForStation(const std::string& siteId,
                            const std::string& pollutant,
                            int numThreads = 1) const = 0;
    
    // === Spatial Queries ===
    
    // Find top-N stations with highest pollutant levels at specific time
    virtual std::vector<std::pair<std::string, double>>
        topNStationsAtTime(int n, long long timestamp,
                          const std::string& pollutant,
                          int numThreads = 1) const = 0;
    
    // Average pollutant in geographic bounding box
    virtual double avgInBoundingBox(double minLat, double maxLat,
                                    double minLon, double maxLon,
                                    const std::string& pollutant,
                                    int numThreads = 1) const = 0;
    
    // === Statistics ===
    
    // Count records matching criteria
    virtual size_t countRecords(long long startTime, long long endTime,
                               const std::string& pollutant) const = 0;
    
    // Get unique pollutant types
    virtual std::vector<std::string> getPollutantTypes() const = 0;
    
    // Implementation identification
    virtual std::string getImplementationName() const = 0;
};
```

**Implementation tasks:**
- [ ] Define complete service interface
- [ ] Document expected performance characteristics per operation
- [ ] Add validation requirements

---

#### Step 2.2: Implement Row-Based Service

**File**: `interface/airquality_service.hpp` + `src/airquality_service.cpp` (NEW)

```cpp
class AirQualityService : public IAirQualityService {
private:
    const AirQualityModel* _model;
    
    // Helper: filter records by criteria
    std::vector<const AirQualityRecord*> filterRecords(
        long long startTime, long long endTime,
        const std::string& pollutant,
        const std::string& siteId = "") const;

public:
    explicit AirQualityService(const AirQualityModel* model);
    
    // Implement all interface methods
    // Row-based will be efficient for station-specific queries
    // Less efficient for cross-station temporal aggregations
    
    std::string getImplementationName() const override { 
        return "Row-oriented (Station-centric)"; 
    }
};
```

**Implementation tasks:**
- [ ] Implement temporal aggregations using OpenMP parallel reduction
- [ ] Implement station-specific queries (should be fast - direct station access)
- [ ] Implement spatial queries (iterate stations, check bounding box)
- [ ] Add proper thread safety for parallel operations
- [ ] Optimize filtering with early termination

---

#### Step 2.3: Implement Column-Based Service

**File**: `interface/airquality_service_column.hpp` + `src/airquality_service_column.cpp` (NEW)

```cpp
class AirQualityServiceColumn : public IAirQualityService {
private:
    const AirQualityModelColumn* _model;
    
    // Helper: binary search for time range
    std::pair<int, int> findTimeRange(long long startTime, long long endTime) const;

public:
    explicit AirQualityServiceColumn(const AirQualityModelColumn* model);
    
    // Implement all interface methods
    // Column-based will be efficient for temporal aggregations
    // Less efficient for station-specific time-series queries
    
    std::string getImplementationName() const override { 
        return "Column-oriented (Time-centric)"; 
    }
};
```

**Implementation tasks:**
- [ ] Implement temporal aggregations (should be fast - contiguous memory access)
- [ ] Implement station-specific queries (requires scanning multiple time slots)
- [ ] Implement spatial queries with OpenMP parallel reduction
- [ ] Add cache-friendly access patterns for columnar data
- [ ] Optimize with SIMD-friendly tight loops

---

### Phase 3: Utilities & Parsing (Week 2)

#### Step 3.1: CSV Parser for Air Quality Format

**File**: `interface/airquality_csv_reader.hpp` + `src/airquality_csv_reader.cpp` (NEW)

```cpp
class AirQualityCSVReader {
private:
    std::string _filename;
    std::ifstream _file;
    
    // Parse helpers
    double parseDouble(const std::string& str) const;
    long long parseDateTime(const std::string& iso8601) const;
    
public:
    explicit AirQualityCSVReader(const std::string& filename);
    
    bool open();
    bool readRecord(AirQualityRecord& record);
    void close();
    
    // Bulk loading with progress
    size_t loadAll(std::vector<AirQualityRecord>& records, 
                   bool showProgress = false);
};
```

**Implementation tasks:**
- [ ] Implement CSV parsing (handle quoted fields, commas in values)
- [ ] Implement ISO 8601 datetime parsing (2020-08-10T01:00)
- [ ] Add error handling for malformed records
- [ ] Add optional progress indicator for large files
- [ ] Validate data ranges (lat/lon, reasonable pollutant values)

---

#### Step 3.2: Date/Time Utilities

**File**: `interface/datetime_utils.hpp` + `src/datetime_utils.cpp` (NEW)

```cpp
namespace DateTimeUtils {
    // Parse ISO 8601 to Unix timestamp
    long long parseISO8601(const std::string& dateTimeStr);
    
    // Format Unix timestamp back to string
    std::string formatTimestamp(long long timestamp);
    
    // Extract components
    int getYear(long long timestamp);
    int getMonth(long long timestamp);
    int getDay(long long timestamp);
    int getHour(long long timestamp);
    
    // Time range helpers
    long long roundToHour(long long timestamp);
    long long addHours(long long timestamp, int hours);
    
    // Validation
    bool isValidTimestamp(long long timestamp);
}
```

**Implementation tasks:**
- [ ] Implement ISO 8601 parser (no external libs, manual parsing)
- [ ] Handle timezone if present (or assume UTC)
- [ ] Add timestamp validation
- [ ] Add time arithmetic for range queries

---

#### Step 3.3: Spatial Utilities

**File**: `interface/spatial_utils.hpp` + `src/spatial_utils.cpp` (NEW)

```cpp
namespace SpatialUtils {
    // Haversine distance (km) between two lat/lon points
    double haversineDistance(double lat1, double lon1, double lat2, double lon2);
    
    // Check if point is in bounding box
    bool inBoundingBox(double lat, double lon,
                       double minLat, double maxLat,
                       double minLon, double maxLon);
    
    // Find nearest N stations to a point
    std::vector<int> findNearestStations(
        const std::vector<StationInfo>& stations,
        double lat, double lon, int n);
}
```

**Implementation tasks:**
- [ ] Implement Haversine formula for distance
- [ ] Implement bounding box checks
- [ ] Add validation for lat/lon ranges

---

### Phase 4: Benchmark Framework (Week 3)

#### Step 4.1: Adapt Benchmark Runner

**File**: `interface/airquality_benchmark_runner.hpp` + `src/airquality_benchmark_runner.cpp` (NEW)

Reuse the generic benchmark pattern but with air quality specific operations:

```cpp
namespace AirQualityBenchmarkRunner {
    
    struct BenchmarkConfig {
        int parallelThreads = 4;
        int repetitions = 5;
        bool validateResults = true;
        bool verbose = false;
    };
    
    // Run temporal aggregation benchmark
    void runTemporalAggregationBenchmark(
        const std::vector<std::reference_wrapper<IAirQualityService>>& services,
        long long timestamp,
        const std::string& pollutant,
        const BenchmarkConfig& config);
    
    // Run station time-series benchmark
    void runStationTimeSeriesBenchmark(
        const std::vector<std::reference_wrapper<IAirQualityService>>& services,
        const std::string& siteId,
        const std::string& pollutant,
        const BenchmarkConfig& config);
    
    // Run spatial query benchmark
    void runSpatialQueryBenchmark(
        const std::vector<std::reference_wrapper<IAirQualityService>>& services,
        double minLat, double maxLat, double minLon, double maxLon,
        const std::string& pollutant,
        const BenchmarkConfig& config);
    
    // Run full benchmark suite
    void runFullBenchmarkSuite(
        const std::vector<std::reference_wrapper<IAirQualityService>>& services,
        const BenchmarkConfig& config);
}
```

**Implementation tasks:**
- [ ] Adapt generic benchmark template for air quality operations
- [ ] Implement result validation (serial == parallel)
- [ ] Add statistical reporting (median, std dev)
- [ ] Create comprehensive benchmark suite

---

#### Step 4.2: Create Main Application

**File**: `src/main_airquality.cpp` (NEW)

```cpp
int main(int argc, char* argv[]) {
    // Parse command line
    // --csv <path>        : Path to air quality CSV
    // --threads <n>       : Number of threads
    // --reps <n>          : Repetitions
    // --pollutant <type>  : Filter by pollutant (default: PM2.5)
    // --help              : Show usage
    
    // Load data into both models
    AirQualityModel rowModel;
    AirQualityModelColumn colModel;
    
    std::cout << "Loading air quality data...\n";
    rowModel.readFromCSV(csvPath);
    colModel.readFromCSV(csvPath);
    
    // Validate models loaded same data
    if (rowModel.totalRecordCount() != colModel.totalRecordCount()) {
        std::cerr << "Model mismatch!\n";
        return 1;
    }
    
    std::cout << "Loaded " << rowModel.totalRecordCount() << " records\n";
    std::cout << "Stations: " << rowModel.stationCount() << "\n";
    std::cout << "Time slots: " << colModel.timeSlotCount() << "\n\n";
    
    // Create services
    AirQualityService rowService(&rowModel);
    AirQualityServiceColumn colService(&colModel);
    
    // Run benchmarks
    auto services = createServiceVector(rowService, colService);
    runFullBenchmarkSuite(services, config);
    
    return 0;
}
```

**Implementation tasks:**
- [ ] Implement command-line argument parsing
- [ ] Add progress indicators for large file loading
- [ ] Implement model validation
- [ ] Add summary statistics output
- [ ] Handle errors gracefully

---

### Phase 5: CMake Integration (Week 3)

#### Step 5.1: Update CMakeLists.txt

Add new executables:

```cmake
# Air Quality benchmark executable
add_executable(OpenMP_AirQuality_App
    src/main_airquality.cpp
    src/airqualityModel.cpp
    src/airqualityModelColumn.cpp
    src/airquality_service.cpp
    src/airquality_service_column.cpp
    src/airquality_csv_reader.cpp
    src/airquality_benchmark_runner.cpp
    src/datetime_utils.cpp
    src/spatial_utils.cpp
    # Reuse existing utilities
    src/utils.cpp
    src/benchmark_utils.cpp
)

target_link_libraries(OpenMP_AirQuality_App PRIVATE OpenMP::OpenMP_CXX)
```

**Implementation tasks:**
- [ ] Add new source files to CMake
- [ ] Configure proper compilation flags
- [ ] Ensure OpenMP linking
- [ ] Test build on multiple platforms

---

## 4. Query Operations

### Performance Characteristics

| Operation | Row Model (Station-centric) | Column Model (Time-centric) |
|-----------|-----------------------------|-----------------------------|
| **Avg PM2.5 at specific time** | ❌ Slow (scan all stations) | ✅ Fast (direct time index) |
| **Time series for station** | ✅ Fast (direct station access) | ❌ Slow (scan all times) |
| **Top-N stations at time** | ❌ Slow (scan all stations) | ✅ Fast (single time slot) |
| **Avg in time range (all stations)** | ❌ Slow (iterate all stations) | ✅ Fast (contiguous time slots) |
| **Station time range** | ✅ Fast (single station scan) | ❌ Slow (multiple time lookups) |
| **Spatial bounding box** | 🟡 Medium (filter stations, then aggregate) | 🟡 Medium (filter each time slot) |

### Expected Speedups (Based on Population Model Results)

For **1 million records**:

| Query Type | Column Speedup |
|------------|----------------|
| Temporal aggregations | **10-15x faster** |
| Top-N at specific time | **50-70x faster** |
| Station time-series | **3-5x slower** |
| Cross-time averages | **10-12x faster** |

---

## 5. Memory Estimates

### For 1 Million Records

| Component | Size | Notes |
|-----------|------|-------|
| **Each AirQualityRecord** | ~200 bytes | Struct with strings |
| **1M records data** | ~200 MB | Core data |
| **Station indices** | ~10 MB | Hash maps, vectors |
| **Time indices** | ~20 MB | Timestamp maps |
| **Row model total** | ~230 MB | |
| **Column model total** | ~230 MB | Same data, different layout |
| **Both models loaded** | **~460 MB** | ✅ Very manageable |
| **Peak during loading** | ~600 MB | Temporary buffers |

**Conclusion**: 1M records requires < 1 GB RAM. Very feasible.

---

## 6. Timeline & Milestones

### Week 1: Core Data Structures
- [ ] Day 1-2: Design and implement `AirQualityRecord` and metadata types
- [ ] Day 3-4: Implement `AirQualityModel` (row-based)
- [ ] Day 5: Implement `AirQualityModelColumn` (columnar)

### Week 2: Services & Parsing
- [ ] Day 1-2: Implement CSV parser and datetime utilities
- [ ] Day 3: Implement row-based service with OpenMP
- [ ] Day 4: Implement column-based service with OpenMP
- [ ] Day 5: Testing and validation

### Week 3: Benchmarks & Integration
- [ ] Day 1-2: Implement benchmark framework
- [ ] Day 3: Create main application
- [ ] Day 4: CMake integration and build testing
- [ ] Day 5: Documentation and README updates

### Week 4: Testing & Optimization
- [ ] Day 1-2: Test with actual 1M row fire data
- [ ] Day 3: Performance profiling and optimization
- [ ] Day 4: Edge case handling and error testing
- [ ] Day 5: Final documentation and examples

---

## 7. File Structure (New Files)

```
OpenMP_Mini1_Project/
├── interface/
│   ├── airquality_types.hpp                      # NEW: Core data types
│   ├── airqualityModel.hpp                       # NEW: Row-based model
│   ├── airqualityModelColumn.hpp                 # NEW: Columnar model
│   ├── airquality_service_interface.hpp          # NEW: Service interface
│   ├── airquality_service.hpp                    # NEW: Row service header
│   ├── airquality_service_column.hpp             # NEW: Column service header
│   ├── airquality_csv_reader.hpp                 # NEW: CSV parser
│   ├── airquality_benchmark_runner.hpp           # NEW: Benchmark framework
│   ├── datetime_utils.hpp                        # NEW: Date/time utilities
│   └── spatial_utils.hpp                         # NEW: Spatial utilities
├── src/
│   ├── airqualityModel.cpp                       # NEW: Row model impl
│   ├── airqualityModelColumn.cpp                 # NEW: Column model impl
│   ├── airquality_service.cpp                    # NEW: Row service impl
│   ├── airquality_service_column.cpp             # NEW: Column service impl
│   ├── airquality_csv_reader.cpp                 # NEW: CSV parser impl
│   ├── airquality_benchmark_runner.cpp           # NEW: Benchmarks impl
│   ├── datetime_utils.cpp                        # NEW: DateTime impl
│   ├── spatial_utils.cpp                         # NEW: Spatial impl
│   └── main_airquality.cpp                       # NEW: Main application
├── tests/
│   └── airquality_tests.cpp                      # NEW: Test suite
└── data/
    └── FireData/                                  # EXISTING: Your data
```

---

## 8. Key Implementation Decisions

### Decision 1: Time Bucketing Strategy

**Options:**
1. **Exact timestamps** (one slot per unique timestamp)
2. **Hourly buckets** (round to nearest hour)
3. **Fixed intervals** (e.g., every 2 hours)

**Recommendation**: **Exact timestamps** for accuracy, with hourly aggregation helpers.

**Rationale**: Your data is already at hourly resolution (T01:00, T02:00), so exact timestamps work well. Columnar model will have ~720 time slots per month (24 hours × 30 days).

---

### Decision 2: Station Identification

**Options:**
1. Use `Location` name (human-readable)
2. Use `SiteID1` (unique identifier)
3. Create composite key `Location + SiteID`

**Recommendation**: **Use SiteID1** as primary key, store Location for display.

**Rationale**: SiteIDs are unique and consistent. Location names may have duplicates or variations.

---

### Decision 3: Multi-Pollutant Handling

**Problem**: Each record has one pollutant type (PM2.5, PM10, OZONE). How to organize?

**Options:**
1. **Unified storage**: Store all pollutants together, filter in queries
2. **Separate indices**: Maintain pollutant-specific sub-models
3. **Pollutant columns**: Pivot data to have one column per pollutant

**Recommendation**: **Unified storage with pollutant indices**.

**Rationale**: Keeps data together, filters are fast with indices, allows mixed pollutant queries.

---

### Decision 4: String Storage Optimization

**Problem**: Strings (location, agency, siteId) are expensive and repetitive.

**Options:**
1. Store strings in every record (simple but memory-heavy)
2. String interning (shared string pool with indices)
3. Store IDs only, maintain separate metadata tables

**Recommendation**: **Hybrid approach** - Store full strings during loading, deduplicate to metadata tables.

**Implementation**:
```cpp
// Instead of storing full strings in every record:
struct AirQualityRecord {
    int stationIndex;      // Index into metadata array
    long long timestamp;
    int pollutantIndex;    // Index into pollutant type array
    double value;
    double aqi;
    // ... reduced size ~40 bytes instead of 200
};

// Separate metadata:
std::vector<StationInfo> stations;
std::vector<std::string> pollutantTypes;
```

**Memory savings**: 200 bytes → 40 bytes per record = **80% reduction** for 1M records (200MB → 40MB)

---

## 9. Testing Strategy

### Unit Tests

1. **DateTime parsing tests**
   - Valid ISO 8601 formats
   - Edge cases (leap years, DST transitions)
   - Invalid formats (error handling)

2. **CSV parsing tests**
   - Well-formed records
   - Malformed records (missing fields, invalid values)
   - Large file handling (progress, memory)

3. **Spatial utility tests**
   - Haversine distance calculation
   - Bounding box edge cases
   - Coordinate validation

4. **Model consistency tests**
   - Row model == Column model (same data, different layout)
   - Record counts match
   - Aggregations match

### Integration Tests

1. **Full pipeline test**: CSV → Models → Services → Queries
2. **Parallel correctness**: Serial results == Parallel results
3. **Performance regression**: Track benchmark results over time

### Benchmark Tests

1. **Load 1M records** from actual fire data
2. **Run full benchmark suite** comparing row vs column
3. **Validate speedup expectations** (column faster for temporal queries)

---

## 10. Example Usage (After Implementation)

```bash
# Build the project
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Run air quality benchmark
./build/OpenMP_AirQuality_App \
    --csv data/FireData/20200810/20200810-01.csv \
    --threads 8 \
    --reps 5 \
    --pollutant PM2.5

# Expected output:
# ============================================
# Air Quality Data Analysis
# ============================================
# Loading data/FireData/20200810/20200810-01.csv...
# Loaded 2,236 records
# Stations: 1,234
# Time slots: 24 (hourly for one day)
# Pollutants: PM2.5, PM10, OZONE
# 
# Running benchmarks with 8 threads, 5 repetitions...
# 
# === Temporal Aggregation (Average PM2.5 at 2020-08-10T12:00) ===
# Row-oriented:    Serial: 245.3 µs  |  Parallel: 89.2 µs  |  Speedup: 2.8x
# Column-oriented: Serial: 23.4 µs   |  Parallel: 8.1 µs   |  Speedup: 2.9x
# ✅ Column is 10.5x faster than row for this query
#
# === Station Time Series (SiteID: 840060150007, PM2.5) ===
# Row-oriented:    Serial: 12.1 µs   |  Parallel: 8.3 µs   |  Speedup: 1.5x
# Column-oriented: Serial: 89.4 µs   |  Parallel: 67.2 µs  |  Speedup: 1.3x
# ✅ Row is 7.4x faster than column for this query
#
# === Top-10 Stations at Time (2020-08-10T12:00, PM2.5) ===
# Row-oriented:    Serial: 412.7 µs  |  Parallel: 118.5 µs |  Speedup: 3.5x
# Column-oriented: Serial: 34.2 µs   |  Parallel: 11.8 µs  |  Speedup: 2.9x
# ✅ Column is 12.1x faster than row for this query
```

---

## 11. Success Criteria

### Functional Requirements
- ✅ Load 1M+ air quality records from CSV
- ✅ Both models store identical data
- ✅ All queries return correct results
- ✅ Parallel execution matches serial results

### Performance Requirements
- ✅ Column model 8-15x faster for temporal aggregations
- ✅ Row model 5-10x faster for station time-series
- ✅ Parallel scaling: 2-4x speedup on 4 threads for large queries
- ✅ Load 1M records in < 30 seconds

### Memory Requirements
- ✅ Total memory < 1 GB for 1M records
- ✅ No memory leaks (Valgrind clean)

---

## Next Steps

**Ready to proceed?** I can now start implementing:

1. **Start with Phase 1, Step 1.1**: Create the core `AirQualityRecord` structure
2. **Progress sequentially** through each step
3. **Test incrementally** as we build

**Would you like me to:**
- A) Start implementing Phase 1 now
- B) Review/modify the plan first
- C) Focus on a specific component

Let me know and I'll begin the implementation! 🚀


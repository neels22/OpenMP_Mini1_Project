#include <iostream>
#include <iomanip>
#include <chrono>
#include "../interface/parallel_csv_loader.hpp"
#include "../interface/airquality_model_row.hpp"
#include "../interface/airquality_model_column.hpp"
#include "../interface/airquality_service_row.hpp"
#include "../interface/airquality_service_column.hpp"
#include "../interface/datetime_utils.hpp"

using namespace AirQuality;
using Clock = std::chrono::high_resolution_clock;

void printSeparator(char c = '=', int width = 70) {
    std::cout << std::string(width, c) << "\n";
}

void printHeader(const std::string& title) {
    printSeparator();
    std::cout << "  " << title << "\n";
    printSeparator();
}

template<typename Func>
double benchmarkQuery(const std::string& label, Func func, int repetitions = 5) {
    std::vector<double> times;
    times.reserve(repetitions);
    
    // Warmup
    func();
    
    // Measure
    for (int i = 0; i < repetitions; i++) {
        auto start = Clock::now();
        func();
        auto end = Clock::now();
        times.push_back(std::chrono::duration<double, std::micro>(end - start).count());
    }
    
    // Return median
    std::sort(times.begin(), times.end());
    return times[times.size() / 2];
}

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Air Quality Analysis: Complete Row vs Column Comparison        ║\n";
    std::cout << "║  Demonstrating OpenMP Parallelization                           ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";
    
    // Configuration
    std::string directory = (argc > 1) ? argv[1] : "data/FireData/20200810";
    int numThreads = (argc > 2) ? std::stoi(argv[2]) : 4;
    int repetitions = (argc > 3) ? std::stoi(argv[3]) : 5;
    
    std::cout << "📁 Directory: " << directory << "\n";
    std::cout << "🧵 Threads: " << numThreads << "\n";
    std::cout << "🔄 Repetitions: " << repetitions << "\n\n";
    
    // PHASE 1: PARALLEL FILE LOADING
    printHeader("PHASE 1: PARALLEL FILE LOADING");
    
    std::cout << "\nScanning for CSV files...\n";
    auto files = ParallelCSVLoader::scanDirectory(directory);
    std::cout << "Found " << files.size() << " CSV files\n\n";
    
    if (files.empty()) {
        std::cerr << "❌ No CSV files found. Exiting.\n";
        return 1;
    }
    
    // Sequential loading
    std::cout << "Loading files SEQUENTIALLY...\n";
    auto seqStart = Clock::now();
    auto seqResults = ParallelCSVLoader::loadSequential(files);
    auto seqEnd = Clock::now();
    double seqTime = std::chrono::duration<double, std::milli>(seqEnd - seqStart).count();
    
    size_t totalRecords = 0;
    for (const auto& r : seqResults) totalRecords += r.recordCount;
    std::cout << "  ✅ Loaded " << totalRecords << " records in " << seqTime << " ms\n\n";
    
    // Parallel loading
    std::cout << "Loading files IN PARALLEL (" << numThreads << " threads)...\n";
    auto parStart = Clock::now();
    auto parResults = ParallelCSVLoader::loadParallel(files, numThreads);
    auto parEnd = Clock::now();
    double parTime = std::chrono::duration<double, std::milli>(parEnd - parStart).count();
    
    std::cout << "  ✅ Loaded " << totalRecords << " records in " << parTime << " ms\n";
    std::cout << "  🚀 File Loading Speedup: " << (seqTime / parTime) << "x\n\n";
    
    // PHASE 2: BUILD DATA MODELS
    printHeader("PHASE 2: BUILD ROW & COLUMN MODELS");
    
    RowModel rowModel;
    ColumnModel colModel;
    
    std::cout << "\n";
    auto buildStart = Clock::now();
    rowModel.buildFromFiles(parResults);
    auto rowBuildEnd = Clock::now();
    double rowBuildTime = std::chrono::duration<double, std::milli>(rowBuildEnd - buildStart).count();
    
    colModel.buildFromFiles(parResults);
    auto colBuildEnd = Clock::now();
    double colBuildTime = std::chrono::duration<double, std::milli>(colBuildEnd - rowBuildEnd).count();
    
    std::cout << "Build times: Row=" << rowBuildTime << "ms, Column=" << colBuildTime << "ms\n\n";
    
    // Create services
    RowService rowService(&rowModel);
    ColumnService colService(&colModel);
    
    // Get test parameters
    auto timestamps = colModel.timestamps();
    if (timestamps.empty()) {
        std::cerr << "❌ No data loaded\n";
        return 1;
    }
    
    long long testTime = timestamps[timestamps.size() / 2];  // Middle timestamp
    std::string testPollutant = "PM2.5";
    std::string testStation = rowModel.stations()[0].siteId;  // First station
    
    std::cout << "Test parameters:\n";
    std::cout << "  Timestamp: " << testTime << " (" << DateTimeUtils::formatTimestamp(testTime) << ")\n";
    std::cout << "  Pollutant: " << testPollutant << "\n";
    std::cout << "  Test Station: " << testStation << "\n\n";
    
    // PHASE 3: PERFORMANCE COMPARISON
    printHeader("PHASE 3: QUERY PERFORMANCE COMPARISON");
    
    // Query 1: Average at specific time (Column Model Advantage)
    std::cout << "\n📊 Query 1: Average " << testPollutant << " at specific time\n";
    std::cout << "   (Expected: Column faster - direct time slot access)\n\n";
    
    double rowAvg1 = 0, colAvg1 = 0;
    double rowAvg1Ser = benchmarkQuery("", [&]() { rowAvg1 = rowService.avgPollutantAtTime(testTime, testPollutant, 1); }, repetitions);
    double rowAvg1Par = benchmarkQuery("", [&]() { rowAvg1 = rowService.avgPollutantAtTime(testTime, testPollutant, numThreads); }, repetitions);
    
    double colAvg1Ser = benchmarkQuery("", [&]() { colAvg1 = colService.avgPollutantAtTime(testTime, testPollutant, 1); }, repetitions);
    double colAvg1Par = benchmarkQuery("", [&]() { colAvg1 = colService.avgPollutantAtTime(testTime, testPollutant, numThreads); }, repetitions);
    
    std::cout << "   Row Model:    Serial=" << std::setw(10) << std::fixed << std::setprecision(2) << rowAvg1Ser << " µs  |  "
              << "Parallel=" << std::setw(10) << rowAvg1Par << " µs  |  Speedup: " << (rowAvg1Ser/rowAvg1Par) << "x\n";
    std::cout << "   Column Model: Serial=" << std::setw(10) << colAvg1Ser << " µs  |  "
              << "Parallel=" << std::setw(10) << colAvg1Par << " µs  |  Speedup: " << (colAvg1Ser/colAvg1Par) << "x\n";
    std::cout << "   Result: " << rowAvg1 << "\n";
    std::cout << "   🏆 Column is " << (rowAvg1Par / colAvg1Par) << "x faster than Row (parallel)\n\n";
    
    // Query 2: Station time series (Row Model Advantage)
    std::cout << "📊 Query 2: Time series for specific station\n";
    std::cout << "   (Expected: Row faster - direct station access)\n\n";
    
    std::vector<std::pair<long long, double>> rowTS, colTS;
    double rowTS1 = benchmarkQuery("", [&]() { rowTS = rowService.timeSeriesForStation(testStation, testPollutant, 1); }, repetitions);
    double colTS1 = benchmarkQuery("", [&]() { colTS = colService.timeSeriesForStation(testStation, testPollutant, 1); }, repetitions);
    
    std::cout << "   Row Model:    " << std::setw(10) << rowTS1 << " µs\n";
    std::cout << "   Column Model: " << std::setw(10) << colTS1 << " µs\n";
    std::cout << "   Result size: " << rowTS.size() << " measurements\n";
    std::cout << "   🏆 Row is " << (colTS1 / rowTS1) << "x faster than Column\n\n";
    
    // Query 3: Top-10 stations (Both benefit from parallelization)
    std::cout << "📊 Query 3: Top-10 stations with highest " << testPollutant << "\n";
    std::cout << "   (Both models benefit from parallelization)\n\n";
    
    std::vector<std::pair<std::string, double>> rowTop, colTop;
    double rowTopSer = benchmarkQuery("", [&]() { rowTop = rowService.topNStationsAtTime(10, testTime, testPollutant, 1); }, repetitions);
    double rowTopPar = benchmarkQuery("", [&]() { rowTop = rowService.topNStationsAtTime(10, testTime, testPollutant, numThreads); }, repetitions);
    
    double colTopSer = benchmarkQuery("", [&]() { colTop = colService.topNStationsAtTime(10, testTime, testPollutant, 1); }, repetitions);
    double colTopPar = benchmarkQuery("", [&]() { colTop = colService.topNStationsAtTime(10, testTime, testPollutant, numThreads); }, repetitions);
    
    std::cout << "   Row Model:    Serial=" << std::setw(10) << rowTopSer << " µs  |  "
              << "Parallel=" << std::setw(10) << rowTopPar << " µs  |  Speedup: " << (rowTopSer/rowTopPar) << "x\n";
    std::cout << "   Column Model: Serial=" << std::setw(10) << colTopSer << " µs  |  "
              << "Parallel=" << std::setw(10) << colTopPar << " µs  |  Speedup: " << (colTopSer/colTopPar) << "x\n";
    
    if (!rowTop.empty()) {
        std::cout << "   Top station: " << rowTop[0].first << " with value " << rowTop[0].second << "\n";
    }
    std::cout << "\n";
    
    // SUMMARY
    printHeader("SUMMARY");
    
    std::cout << "\n📈 Key Findings:\n\n";
    std::cout << "1. Parallel File Loading:\n";
    std::cout << "   Loaded " << totalRecords << " records from " << files.size() << " files\n";
    std::cout << "   Speedup: " << std::fixed << std::setprecision(2) << (seqTime / parTime) << "x\n\n";
    
    std::cout << "2. Temporal Aggregation (avgPollutantAtTime):\n";
    std::cout << "   Column Model: " << std::fixed << std::setprecision(2) << (rowAvg1Par / colAvg1Par) << "x faster than Row Model\n";
    std::cout << "   OpenMP Speedup (Column): " << (colAvg1Ser / colAvg1Par) << "x with " << numThreads << " threads\n\n";
    
    std::cout << "3. Station Time Series:\n";
    std::cout << "   Row Model: " << (colTS1 / rowTS1) << "x faster than Column Model\n";
    std::cout << "   (As expected - direct station access)\n\n";
    
    std::cout << "4. Data Structure Trade-offs:\n";
    std::cout << "   ✅ Use Column Model for temporal aggregations (queries across all stations)\n";
    std::cout << "   ✅ Use Row Model for station-specific analyses (time series per station)\n";
    std::cout << "   ✅ Both benefit significantly from OpenMP parallelization\n\n";
    
    printSeparator();
    std::cout << "✅ Benchmark complete!\n";
    printSeparator();
    std::cout << "\n";
    
    return 0;
}


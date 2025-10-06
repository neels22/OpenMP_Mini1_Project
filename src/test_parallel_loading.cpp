#include <iostream>
#include <iomanip>
#include "../interface/parallel_csv_loader.hpp"

/**
 * @file test_parallel_loading.cpp
 * @brief Quick test of parallel file loading
 * 
 * Tests the parallel CSV loader with actual FireData files to demonstrate:
 * 1. Successful file loading
 * 2. Sequential vs parallel performance comparison
 * 3. OpenMP speedup measurement
 */

using namespace AirQuality;

void printSeparator() {
    std::cout << std::string(70, '=') << "\n";
}

void printResults(const std::vector<FileLoadResult>& results) {
    size_t totalRecords = 0;
    size_t successCount = 0;
    double totalTime = 0.0;
    
    for (const auto& result : results) {
        totalRecords += result.recordCount;
        totalTime += result.loadTimeMs;
        if (result.success) successCount++;
        
        std::cout << "  📄 " << std::filesystem::path(result.filename).filename().string();
        if (result.success) {
            std::cout << ": " << result.recordCount << " records in " 
                      << std::fixed << std::setprecision(2) << result.loadTimeMs << " ms";
            if (!result.errorMsg.empty()) {
                std::cout << " ⚠️  " << result.errorMsg;
            }
        } else {
            std::cout << ": ❌ FAILED - " << result.errorMsg;
        }
        std::cout << "\n";
    }
    
    std::cout << "\n  Summary: " << successCount << "/" << results.size() 
              << " files loaded successfully\n";
    std::cout << "  Total records: " << totalRecords << "\n";
    std::cout << "  Total time: " << std::fixed << std::setprecision(2) << totalTime << " ms\n";
}

int main(int argc, char* argv[]) {
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║     Parallel File Loading Test                          ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n\n";
    
    // Default to first day of FireData if no argument provided
    std::string directory = (argc > 1) ? argv[1] : "data/FireData/20200810";
    int numThreads = (argc > 2) ? std::stoi(argv[2]) : 4;
    
    std::cout << "📁 Directory: " << directory << "\n";
    std::cout << "🧵 Threads: " << numThreads << "\n\n";
    
    // Scan for CSV files
    printSeparator();
    std::cout << "Scanning for CSV files...\n";
    auto files = ParallelCSVLoader::scanDirectory(directory);
    
    if (files.empty()) {
        std::cerr << "❌ No CSV files found in " << directory << "\n";
        std::cerr << "Usage: " << argv[0] << " <directory> [num_threads]\n";
        std::cerr << "Example: " << argv[0] << " data/FireData/20200810 8\n";
        return 1;
    }
    
    std::cout << "Found " << files.size() << " CSV files:\n";
    for (const auto& file : files) {
        std::cout << "  • " << std::filesystem::path(file).filename().string() << "\n";
    }
    std::cout << "\n";
    
    // Test 1: Sequential Loading
    printSeparator();
    std::cout << "TEST 1: SEQUENTIAL LOADING\n";
    printSeparator();
    
    auto seqStart = std::chrono::high_resolution_clock::now();
    auto seqResults = ParallelCSVLoader::loadSequential(files);
    auto seqEnd = std::chrono::high_resolution_clock::now();
    double seqTotalTime = std::chrono::duration<double, std::milli>(seqEnd - seqStart).count();
    
    std::cout << "\nResults:\n";
    printResults(seqResults);
    std::cout << "\n⏱️  Wall-clock time: " << std::fixed << std::setprecision(2) 
              << seqTotalTime << " ms\n\n";
    
    // Test 2: Parallel Loading
    printSeparator();
    std::cout << "TEST 2: PARALLEL LOADING (" << numThreads << " threads)\n";
    printSeparator();
    
    auto parStart = std::chrono::high_resolution_clock::now();
    auto parResults = ParallelCSVLoader::loadParallel(files, numThreads);
    auto parEnd = std::chrono::high_resolution_clock::now();
    double parTotalTime = std::chrono::duration<double, std::milli>(parEnd - parStart).count();
    
    std::cout << "\nResults:\n";
    printResults(parResults);
    std::cout << "\n⏱️  Wall-clock time: " << std::fixed << std::setprecision(2) 
              << parTotalTime << " ms\n\n";
    
    // Comparison
    printSeparator();
    std::cout << "PERFORMANCE COMPARISON\n";
    printSeparator();
    
    double speedup = seqTotalTime / parTotalTime;
    
    std::cout << "\n";
    std::cout << "  Sequential time:  " << std::setw(10) << std::fixed << std::setprecision(2) 
              << seqTotalTime << " ms\n";
    std::cout << "  Parallel time:    " << std::setw(10) << std::fixed << std::setprecision(2) 
              << parTotalTime << " ms\n";
    std::cout << "  ────────────────────────────\n";
    std::cout << "  🚀 Speedup:       " << std::setw(10) << std::fixed << std::setprecision(2) 
              << speedup << "x\n\n";
    
    if (speedup > 3.0) {
        std::cout << "  ✅ Excellent speedup! OpenMP parallelization is working great!\n";
    } else if (speedup > 1.5) {
        std::cout << "  ✅ Good speedup! Parallel loading is faster than sequential.\n";
    } else {
        std::cout << "  ⚠️  Limited speedup. Possible reasons:\n";
        std::cout << "     - Small files (I/O overhead dominates)\n";
        std::cout << "     - Few files (not enough parallelism)\n";
        std::cout << "     - Disk I/O bottleneck\n";
    }
    std::cout << "\n";
    
    // Show sample record
    if (!parResults.empty() && !parResults[0].records.empty()) {
        printSeparator();
        std::cout << "SAMPLE RECORD\n";
        printSeparator();
        std::cout << "\n" << parResults[0].records[0].toString() << "\n";
        std::cout << "  Latitude:  " << parResults[0].records[0].latitude << "\n";
        std::cout << "  Longitude: " << parResults[0].records[0].longitude << "\n";
        std::cout << "  Timestamp: " << parResults[0].records[0].timestamp << "\n";
        std::cout << "  SiteID:    " << parResults[0].records[0].siteId1 << "\n\n";
    }
    
    printSeparator();
    std::cout << "✅ Test complete!\n";
    printSeparator();
    
    return 0;
}


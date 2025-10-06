#pragma once

#include "airquality_types.hpp"
#include <vector>
#include <string>
#include <filesystem>

/**
 * @file parallel_csv_loader.hpp
 * @brief Parallel CSV file loader using OpenMP
 * 
 * Provides sequential and parallel file loading capabilities to demonstrate
 * OpenMP performance benefits. Each file is loaded independently by a thread,
 * making this an ideal use case for file-level parallelization.
 */

namespace AirQuality {

/**
 * @class ParallelCSVLoader
 * @brief Load multiple CSV files sequentially or in parallel
 * 
 * Demonstrates OpenMP parallelization at the file level. Each thread loads
 * a complete file independently with no shared state during loading, making
 * this a perfect embarrassingly parallel problem.
 */
class ParallelCSVLoader {
public:
    /**
     * @brief Load a single CSV file (thread-safe)
     * @param filepath Path to CSV file
     * @return FileLoadResult containing records, timing, and status
     * 
     * Thread-safe file loading function. Each thread calls this independently
     * on different files with no locks needed.
     * 
     * CSV Format Expected:
     * Lat,Lon,DateTime,Pollutant,Value,Unit,AQI,AQICategory,QualityFlag,Location,Agency,SiteID1,SiteID2
     */
    static FileLoadResult loadFile(const std::string& filepath);
    
    /**
     * @brief Load multiple files SEQUENTIALLY (baseline for comparison)
     * @param filepaths Vector of file paths to load
     * @return Vector of FileLoadResult, one per file
     * 
     * Loads files one-by-one in order. Used as baseline to measure
     * parallel speedup.
     */
    static std::vector<FileLoadResult> loadSequential(
        const std::vector<std::string>& filepaths);
    
    /**
     * @brief Load multiple files IN PARALLEL using OpenMP
     * @param filepaths Vector of file paths to load
     * @param numThreads Number of OpenMP threads to use
     * @return Vector of FileLoadResult, one per file
     * 
     * KEY PARALLELIZATION: Uses OpenMP to load multiple files simultaneously.
     * Each thread loads a complete file independently.
     * 
     * Thread Safety: Safe because each thread writes to a different position
     * in the pre-allocated results vector. No locks needed!
     * 
     * Performance: Expect 4-8x speedup with 8 threads on 12+ files
     * (limited by I/O bandwidth and CPU cores).
     */
    static std::vector<FileLoadResult> loadParallel(
        const std::vector<std::string>& filepaths,
        int numThreads = 4);
    
    /**
     * @brief Scan directory for all CSV files
     * @param directory Path to directory
     * @return Sorted vector of CSV file paths
     * 
     * Recursively finds all .csv files in directory and returns sorted list.
     */
    static std::vector<std::string> scanDirectory(const std::string& directory);
    
    /**
     * @brief Get CSV files matching a pattern
     * @param directory Directory to search
     * @param pattern Glob pattern (e.g., "20200810-*.csv")
     * @return Vector of matching file paths
     * 
     * Filters CSV files by filename pattern.
     */
    static std::vector<std::string> scanDirectoryPattern(
        const std::string& directory,
        const std::string& pattern);

private:
    /**
     * @brief Parse a single CSV line into a Record
     * @param line CSV line to parse
     * @return Parsed Record
     * @throws std::runtime_error if parsing fails
     * 
     * Handles quoted fields and embedded commas.
     */
    static Record parseLine(const std::string& line);
    
    /**
     * @brief Split CSV line handling quotes
     * @param line Line to split
     * @return Vector of field values
     * 
     * Properly handles:
     * - Quoted fields: "value,with,commas"
     * - Escaped quotes: "value with ""quotes"" inside"
     * - Empty fields
     */
    static std::vector<std::string> splitCSV(const std::string& line);
    
    /**
     * @brief Trim whitespace from string
     * @param str String to trim
     * @return Trimmed string
     */
    static std::string trim(const std::string& str);
    
    /**
     * @brief Remove quotes from quoted field
     * @param str String to unquote
     * @return Unquoted string
     */
    static std::string unquote(const std::string& str);
};

} // namespace AirQuality


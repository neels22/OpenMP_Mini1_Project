#pragma once

#include <cstddef>

/**
 * @file constants.hpp
 * @brief Centralized configuration constants for the population analysis project
 * 
 * This file contains all configuration constants used throughout the project,
 * eliminating magic numbers and providing a single source of truth for
 * configurable parameters. All constants are in the Config namespace to
 * avoid naming conflicts.
 */

namespace Config {
    // === Memory Management Configuration ===
    
    /// Default reserve size for column vectors in columnar model
    /// Optimizes memory allocation by pre-reserving space for typical datasets
    constexpr std::size_t DEFAULT_COLUMN_RESERVE_SIZE = 1024;
    
    // === Synthetic Data Generation Configuration ===
    
    /// Default number of countries to generate in synthetic datasets
    /// Large enough to stress-test parallel algorithms while remaining manageable
    constexpr std::size_t DEFAULT_SYNTHETIC_ROWS = 200000;
    
    /// Default number of years to generate in synthetic datasets
    /// Represents a typical multi-decade time series
    constexpr std::size_t DEFAULT_SYNTHETIC_YEARS = 50;
    
    /// Starting year for synthetic data generation
    /// Base year from which synthetic time series begins
    constexpr int DEFAULT_BASE_YEAR = 2000;
    
    /// Random number generator seed for reproducible synthetic data
    /// Fixed seed ensures consistent synthetic datasets across runs
    constexpr int DEFAULT_RNG_SEED = 123456;
    
    // === Benchmark Configuration ===
    
    /// Default number of repetitions for timing measurements
    /// Balances statistical accuracy with execution time
    constexpr int DEFAULT_REPETITIONS = 5;
    
    /// Fallback thread count when hardware detection fails
    /// Conservative default for systems where std::thread::hardware_concurrency() returns 0
    constexpr int DEFAULT_THREADS_FALLBACK = 4;
    
    /// Default number of threads for parallel benchmarks
    constexpr int DEFAULT_PARALLEL_THREADS = 4;
    
    /// Default number for top-N operations
    constexpr int TOP_N_DEFAULT = 10;
    
    // === CSV Parsing Configuration ===
    
    /// Default field delimiter for CSV parsing
    constexpr char DEFAULT_CSV_DELIMITER = ',';
    
    /// Default quote character for CSV parsing
    /// Used to handle fields containing delimiter characters
    constexpr char DEFAULT_CSV_QUOTE = '"';
    
    /// Default comment character for CSV parsing
    /// Lines starting with this character are ignored
    constexpr char DEFAULT_CSV_COMMENT = '#';
}
#pragma once

#include <cstddef>

namespace Config {
    // Column storage configuration
    constexpr std::size_t DEFAULT_COLUMN_RESERVE_SIZE = 1024;
    
    // Synthetic data generation defaults
    constexpr std::size_t DEFAULT_SYNTHETIC_ROWS = 200000;
    constexpr std::size_t DEFAULT_SYNTHETIC_YEARS = 50;
    constexpr int DEFAULT_BASE_YEAR = 2000;
    constexpr int DEFAULT_RNG_SEED = 123456;
    
    // Benchmark defaults
    constexpr int DEFAULT_REPETITIONS = 5;
    constexpr int DEFAULT_THREADS_FALLBACK = 4;
    
    // CSV parsing defaults
    constexpr char DEFAULT_CSV_DELIMITER = ',';
    constexpr char DEFAULT_CSV_QUOTE = '"';
    constexpr char DEFAULT_CSV_COMMENT = '#';
}
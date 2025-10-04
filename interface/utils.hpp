#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>

/**
 * @file utils.hpp
 * @brief Common utility functions for timing, statistics, and data parsing
 * 
 * This file provides shared utilities used throughout the population analysis
 * project. By centralizing these functions, we eliminate code duplication
 * and ensure consistent behavior across different modules.
 */

namespace Utils {
    /**
     * @brief Parse a string to long long, returning 0 on any error
     * @param s Input string to parse
     * @return Parsed long long value, or 0 if parsing fails
     * 
     * Handles leading/trailing whitespace gracefully and provides
     * safe parsing with consistent error handling across the project.
     */
    long long parseLongOrZero(const std::string& s);

    // === Timing Utilities ===
    
    /// High-resolution clock type for consistent timing measurements
    using Clock = std::chrono::high_resolution_clock;
    
    /**
     * @brief Time the execution of a function and return elapsed time in microseconds
     * @param f Function to execute and time
     * @return Elapsed time in microseconds (double precision)
     * 
     * Provides high-precision timing for performance analysis.
     * Uses std::chrono::high_resolution_clock for maximum accuracy.
     */
    double timeCall(const std::function<void()>& f);
    
    /**
     * @brief Run a function multiple times and return vector of elapsed times
     * @param f Function to execute and time
     * @param runs Number of times to execute the function
     * @return Vector of elapsed times in microseconds
     * 
     * Useful for statistical analysis of function performance.
     * Each execution is timed independently to capture variance.
     */
    std::vector<double> timeCallMulti(const std::function<void()>& f, int runs);

    // === Statistical Utilities ===
    
    /**
     * @brief Calculate median of a vector of values
     * @param v Vector of values (copied for sorting)
     * @return Median value, or 0.0 if vector is empty
     * 
     * Sorts the input vector and returns the middle value.
     * For even-sized vectors, returns the average of the two middle values.
     */
    double median(std::vector<double> v);
    
    /**
     * @brief Calculate standard deviation of a vector of values
     * @param v Vector of values (const reference)
     * @return Sample standard deviation, or 0.0 if fewer than 2 values
     * 
     * Uses the sample standard deviation formula (N-1 denominator)
     * for unbiased estimation from sample data.
     */
    double stddev(const std::vector<double>& v);
}
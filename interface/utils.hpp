#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace Utils {
    /**
     * Parse a string to long long, returning 0 on any error.
     * Handles leading/trailing whitespace gracefully.
     */
    long long parseLongOrZero(const std::string& s);

    // Timing utilities
    using Clock = std::chrono::high_resolution_clock;
    
    /**
     * Time the execution of a function and return elapsed time in microseconds
     */
    double timeCall(const std::function<void()>& f);
    
    /**
     * Run a function multiple times and return vector of elapsed times in microseconds
     */
    std::vector<double> timeCallMulti(const std::function<void()>& f, int runs);

    // Statistical utilities
    /**
     * Calculate median of a vector of values
     */
    double median(std::vector<double> v);
    
    /**
     * Calculate standard deviation of a vector of values
     */
    double stddev(const std::vector<double>& v);
}
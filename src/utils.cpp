/**
 * @file utils.cpp
 * @brief Implementation of common utility functions for timing, statistics, and parsing
 * 
 * This file implements the utility functions declared in utils.hpp, providing
 * robust implementations with proper error handling and optimal performance
 * characteristics. These utilities are used throughout the project to ensure
 * consistent behavior and eliminate code duplication.
 */

#include "../interface/utils.hpp"
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace Utils {
    long long parseLongOrZero(const std::string& s) {
        try {
            size_t idx = 0;
            long long v = std::stoll(s, &idx);
            return v;
        } catch (...) {
            // Return 0 for any parsing error (invalid format, overflow, etc.)
            return 0;
        }
    }

    double timeCall(const std::function<void()>& f) {
        // Use high-resolution clock for maximum timing precision
        auto t0 = Clock::now();
        f();
        auto t1 = Clock::now();
        
        // Convert to microseconds for consistent reporting across the project
        std::chrono::duration<double, std::micro> d = t1 - t0;
        return d.count();
    }

    std::vector<double> timeCallMulti(const std::function<void()>& f, int runs) {
        std::vector<double> res;
        res.reserve(static_cast<std::size_t>(runs));
        
        // Execute function multiple times for statistical analysis
        for (int i = 0; i < runs; ++i) {
            res.push_back(timeCall(f));
        }
        return res;
    }

    double median(std::vector<double> v) {
        if (v.empty()) return 0.0;
        
        // Sort vector to find middle value(s)
        std::sort(v.begin(), v.end());
        size_t m = v.size() / 2;
        
        if (v.size() % 2 == 1) {
            // Odd number of elements: return middle element
            return v[m];
        }
        // Even number of elements: return average of two middle elements
        return 0.5 * (v[m-1] + v[m]);
    }

    double stddev(const std::vector<double>& v) {
        if (v.size() < 2) return 0.0;
        
        // Calculate sample mean
        double mean = 0.0;
        for (double x : v) {
            mean += x;
        }
        mean /= static_cast<double>(v.size());
        
        // Calculate sample variance (using N-1 denominator for unbiased estimate)
        double s = 0.0;
        for (double x : v) {
            s += (x - mean) * (x - mean);
        }
        return std::sqrt(s / static_cast<double>(v.size() - 1));
    }
}
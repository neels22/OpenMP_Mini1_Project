#pragma once

#include <string>
#include <vector>
#include <utility>

/**
 * @file airquality_service_interface.hpp
 * @brief Common interface for air quality data services
 * 
 * Defines a unified interface that both row-oriented and column-oriented
 * services implement, enabling polymorphic benchmarking and easy comparison.
 */

namespace AirQuality {

/**
 * @interface IAirQualityService
 * @brief Abstract interface for air quality queries
 * 
 * All query operations support both serial (numThreads=1) and parallel execution,
 * allowing direct performance comparison.
 */
class IAirQualityService {
public:
    virtual ~IAirQualityService() = default;
    
    // === Temporal Aggregations (Main Use Case for Column Model) ===
    
    /**
     * @brief Calculate average pollutant value at specific timestamp
     * @param timestamp Unix timestamp
     * @param pollutant Pollutant type (e.g., "PM2.5")
     * @param numThreads Number of OpenMP threads (1 for serial)
     * @return Average value, or 0.0 if no matching records
     * 
     * Performance: Column model expected 8-12x faster (direct time slot access)
     */
    virtual double avgPollutantAtTime(
        long long timestamp,
        const std::string& pollutant,
        int numThreads = 1) const = 0;
    
    /**
     * @brief Find maximum pollutant value at specific timestamp
     * @param timestamp Unix timestamp
     * @param pollutant Pollutant type
     * @param numThreads Number of OpenMP threads
     * @return Maximum value, or 0.0 if no matching records
     */
    virtual double maxPollutantAtTime(
        long long timestamp,
        const std::string& pollutant,
        int numThreads = 1) const = 0;
    
    /**
     * @brief Find minimum pollutant value at specific timestamp
     * @param timestamp Unix timestamp
     * @param pollutant Pollutant type
     * @param numThreads Number of OpenMP threads
     * @return Minimum value, or 0.0 if no matching records
     */
    virtual double minPollutantAtTime(
        long long timestamp,
        const std::string& pollutant,
        int numThreads = 1) const = 0;
    
    // === Station-Specific Queries (Main Use Case for Row Model) ===
    
    /**
     * @brief Get time series for specific station
     * @param siteId Station identifier
     * @param pollutant Pollutant type
     * @param numThreads Number of OpenMP threads
     * @return Vector of (timestamp, value) pairs
     * 
     * Performance: Row model expected 5-8x faster (direct station access)
     */
    virtual std::vector<std::pair<long long, double>> timeSeriesForStation(
        const std::string& siteId,
        const std::string& pollutant,
        int numThreads = 1) const = 0;
    
    /**
     * @brief Calculate average for station over time range
     * @param siteId Station identifier
     * @param startTime Start timestamp (inclusive)
     * @param endTime End timestamp (inclusive)
     * @param pollutant Pollutant type
     * @param numThreads Number of OpenMP threads
     * @return Average value, or 0.0 if no matching records
     */
    virtual double avgForStationInRange(
        const std::string& siteId,
        long long startTime,
        long long endTime,
        const std::string& pollutant,
        int numThreads = 1) const = 0;
    
    // === Top-N Queries (Good for Both Models with OpenMP) ===
    
    /**
     * @brief Find top N stations with highest pollutant levels at timestamp
     * @param n Number of top results
     * @param timestamp Unix timestamp
     * @param pollutant Pollutant type
     * @param numThreads Number of OpenMP threads
     * @return Vector of (siteId, value) pairs, sorted descending
     * 
     * Uses per-thread min-heaps for efficient parallel top-N.
     * Performance: Both models benefit from parallelization, column slightly faster.
     */
    virtual std::vector<std::pair<std::string, double>> topNStationsAtTime(
        int n,
        long long timestamp,
        const std::string& pollutant,
        int numThreads = 1) const = 0;
    
    // === Statistics ===
    
    /**
     * @brief Count records matching criteria
     * @param startTime Start timestamp
     * @param endTime End timestamp
     * @param pollutant Pollutant type
     * @return Number of matching records
     */
    virtual size_t countRecords(
        long long startTime,
        long long endTime,
        const std::string& pollutant) const = 0;
    
    // === Metadata ===
    
    /**
     * @brief Get implementation name for identification
     * @return Human-readable implementation name
     */
    virtual std::string getImplementationName() const = 0;
};

} // namespace AirQuality


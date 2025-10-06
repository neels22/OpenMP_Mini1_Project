#pragma once

#include <vector>
#include <string>
#include <utility>

/**
 * @file fire_service_interface.hpp
 * @brief Common interface for all fire data analytics services
 * 
 * This interface defines the contract that all fire analytics services must implement,
 * enabling polymorphic usage and eliminating code duplication in benchmarks and client code.
 * Both row-oriented and column-oriented fire services implement this interface.
 */

/**
 * @interface IFireService
 * @brief Abstract interface for fire data analytics operations
 * 
 * This interface provides a common contract for fire analytics services,
 * enabling clean separation between business logic and data layout implementations.
 * All implementations must support both serial and parallel execution modes.
 * 
 * Key Benefits:
 * - Eliminates duplicate benchmark code
 * - Enables polymorphic service usage
 * - Enforces consistent API across implementations
 * - Facilitates testing and mocking
 */
class IFireService {
public:
    virtual ~IFireService() = default;

    // === Aggregation Operations by Parameter ===
    
    /// Calculate average concentration for a specific parameter (e.g., "PM2.5")
    /// @param parameter Parameter type (PM2.5, PM10, OZONE, etc.)
    /// @param numThreads Number of threads for parallel execution (1 = serial)
    /// @return Average concentration or 0.0 if parameter not found
    virtual double averageConcentrationForParameter(const std::string& parameter, int numThreads = 1) const = 0;
    
    /// Calculate sum of concentrations for a specific parameter
    /// @param parameter Parameter type (PM2.5, PM10, OZONE, etc.)
    /// @param numThreads Number of threads for parallel execution (1 = serial)
    /// @return Sum of concentrations or 0.0 if parameter not found
    virtual double sumConcentrationsForParameter(const std::string& parameter, int numThreads = 1) const = 0;
    
    /// Find maximum concentration for a specific parameter
    /// @param parameter Parameter type (PM2.5, PM10, OZONE, etc.)
    /// @param numThreads Number of threads for parallel execution (1 = serial)
    /// @return Maximum concentration or 0.0 if parameter not found
    virtual double maxConcentrationForParameter(const std::string& parameter, int numThreads = 1) const = 0;
    
    /// Find minimum concentration for a specific parameter
    /// @param parameter Parameter type (PM2.5, PM10, OZONE, etc.)
    /// @param numThreads Number of threads for parallel execution (1 = serial)
    /// @return Minimum concentration or 0.0 if parameter not found
    virtual double minConcentrationForParameter(const std::string& parameter, int numThreads = 1) const = 0;

    // === AQI Operations ===
    
    /// Calculate average AQI across all measurements
    /// @param numThreads Number of threads for parallel execution (1 = serial)
    /// @return Average AQI or 0.0 if no measurements
    virtual double averageAQI(int numThreads = 1) const = 0;
    
    /// Find maximum AQI across all measurements
    /// @param numThreads Number of threads for parallel execution (1 = serial)
    /// @return Maximum AQI or 0 if no measurements
    virtual int maxAQI(int numThreads = 1) const = 0;
    
    /// Find minimum AQI across all measurements
    /// @param numThreads Number of threads for parallel execution (1 = serial)
    /// @return Minimum AQI or 0 if no measurements
    virtual int minAQI(int numThreads = 1) const = 0;
    
    /// Calculate average AQI for a specific parameter
    /// @param parameter Parameter type (PM2.5, PM10, OZONE, etc.)
    /// @param numThreads Number of threads for parallel execution (1 = serial)
    /// @return Average AQI for parameter or 0.0 if parameter not found
    virtual double averageAQIForParameter(const std::string& parameter, int numThreads = 1) const = 0;

    // === Site-Specific Operations ===
    
    /// Get average concentration for a specific site
    /// @param siteName Name of the monitoring site
    /// @param numThreads Number of threads for parallel execution
    /// @return Average concentration at site or 0.0 if site not found
    virtual double averageConcentrationForSite(const std::string& siteName, int numThreads = 1) const = 0;
    
    /// Get count of measurements for a specific site
    /// @param siteName Name of the monitoring site
    /// @return Number of measurements or 0 if site not found
    virtual std::size_t measurementCountForSite(const std::string& siteName) const = 0;

    // === Geographic Operations ===
    
    /// Count measurements within a geographic bounding box
    /// @param minLat Minimum latitude
    /// @param maxLat Maximum latitude
    /// @param minLon Minimum longitude
    /// @param maxLon Maximum longitude
    /// @param numThreads Number of threads for parallel execution
    /// @return Count of measurements within bounds
    virtual std::size_t countMeasurementsInBounds(double minLat, double maxLat, 
                                                   double minLon, double maxLon,
                                                   int numThreads = 1) const = 0;
    
    /// Calculate average concentration within a geographic bounding box
    /// @param minLat Minimum latitude
    /// @param maxLat Maximum latitude
    /// @param minLon Minimum longitude
    /// @param maxLon Maximum longitude
    /// @param numThreads Number of threads for parallel execution
    /// @return Average concentration within bounds or 0.0 if no measurements
    virtual double averageConcentrationInBounds(double minLat, double maxLat, 
                                                double minLon, double maxLon,
                                                int numThreads = 1) const = 0;

    // === Top-N Operations ===
    
    /// Find top N sites by average concentration
    /// @param n Number of top sites to return
    /// @param numThreads Number of threads for parallel execution
    /// @return Vector of (site_name, avg_concentration) pairs sorted descending
    virtual std::vector<std::pair<std::string, double>> topNSitesByAverageConcentration(std::size_t n, int numThreads = 1) const = 0;
    
    /// Find top N sites by maximum AQI
    /// @param n Number of top sites to return
    /// @param numThreads Number of threads for parallel execution
    /// @return Vector of (site_name, max_aqi) pairs sorted descending
    virtual std::vector<std::pair<std::string, int>> topNSitesByMaxAQI(std::size_t n, int numThreads = 1) const = 0;

    // === Category Analysis ===
    
    /// Count measurements by AQI category (0=Good, 1=Moderate, 2=USG, 3=Unhealthy, 4=Very Unhealthy, 5=Hazardous)
    /// @param category AQI category to count
    /// @param numThreads Number of threads for parallel execution
    /// @return Count of measurements in category
    virtual std::size_t countMeasurementsByCategory(int category, int numThreads = 1) const = 0;
    
    /// Get distribution of measurements across AQI categories
    /// @param numThreads Number of threads for parallel execution
    /// @return Vector of 6 counts (one per category 0-5)
    virtual std::vector<std::size_t> categoryDistribution(int numThreads = 1) const = 0;

    // === Metadata Operations ===
    
    /// Get a human-readable name for this service implementation
    /// @return Implementation name (e.g., "Fire Row-oriented", "Fire Column-oriented")
    virtual std::string getImplementationName() const = 0;
    
    /// Get total number of measurements
    /// @return Total measurement count
    virtual std::size_t totalMeasurementCount() const = 0;
    
    /// Get total number of unique sites
    /// @return Unique site count
    virtual std::size_t uniqueSiteCount() const = 0;
};


#pragma once

#include "fireRowModel.hpp"
#include "fireColumnModel.hpp"
#include <vector>
#include <string>
#include <utility>

/**
 * @file fire_service_direct.hpp
 * @brief Direct (non-virtual) fire analytics services
 * 
 * Simple, direct implementations without inheritance or virtual interfaces.
 * Provides 4 core operations: maxAQI, minAQI, averageAQI, topNSitesByAverageConcentration
 * Following the same pattern as PopulationModelService but for fire data.
 */

/**
 * @class FireRowService
 * @brief Simple fire analytics service using row-oriented data model
 * 
 * Direct implementation without virtual inheritance. Provides analytics operations
 * on FireRowModel with both serial and parallel execution modes.
 */
class FireRowService {
private:
    const FireRowModel* model_;  ///< Pointer to the underlying data model

public:
    /// Constructor
    explicit FireRowService(const FireRowModel* model);
    
    /// Destructor
    ~FireRowService();

    // === Core Analytics Operations ===
    
    /// Find maximum AQI across all measurements
    int maxAQI(int numThreads = 1) const;
    
    /// Find minimum AQI across all measurements
    int minAQI(int numThreads = 1) const;
    
    /// Calculate average AQI across all measurements
    double averageAQI(int numThreads = 1) const;
    
    /// Find top N sites by average concentration
    std::vector<std::pair<std::string, double>> topNSitesByAverageConcentration(std::size_t n, int numThreads = 1) const;
    
    // === Metadata Operations ===
    
    /// Get implementation name
    std::string getImplementationName() const;
    
    /// Get total number of measurements
    std::size_t totalMeasurementCount() const;
    
    /// Get total number of unique sites
    std::size_t uniqueSiteCount() const;
};

/**
 * @class FireColumnService
 * @brief Simple fire analytics service using column-oriented data model
 * 
 * Direct implementation without virtual inheritance. Provides analytics operations
 * on FireColumnModel with both serial and parallel execution modes.
 */
class FireColumnService {
private:
    const FireColumnModel* model_;  ///< Pointer to the underlying data model

public:
    /// Constructor
    explicit FireColumnService(const FireColumnModel* model);
    
    /// Destructor
    ~FireColumnService();

    // === Core Analytics Operations ===
    
    /// Find maximum AQI across all measurements
    int maxAQI(int numThreads = 1) const;
    
    /// Find minimum AQI across all measurements
    int minAQI(int numThreads = 1) const;
    
    /// Calculate average AQI across all measurements
    double averageAQI(int numThreads = 1) const;
    
    /// Find top N sites by average concentration
    std::vector<std::pair<std::string, double>> topNSitesByAverageConcentration(std::size_t n, int numThreads = 1) const;
    
    // === Metadata Operations ===
    
    /// Get implementation name
    std::string getImplementationName() const;
    
    /// Get total number of measurements
    std::size_t totalMeasurementCount() const;
    
    /// Get total number of unique sites
    std::size_t uniqueSiteCount() const;
};
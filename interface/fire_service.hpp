#pragma once

#include "fire_service_interface.hpp"
#include "fireRowModel.hpp"
#include "fireColumnModel.hpp"
#include <memory>

/**
 * @file fire_service.hpp
 * @brief Concrete implementations of fire analytics services
 * 
 * This file provides two concrete implementations:
 * 1. FireRowModelService - Analytics over row-oriented fire data
 * 2. FireColumnModelService - Analytics over column-oriented fire data
 * 
 * Both services implement IFireService and provide identical analytics operations
 * with both serial and parallel (OpenMP) execution modes.
 */

/**
 * @class FireRowModelService
 * @brief Fire analytics service using row-oriented data model
 * 
 * Implements analytics operations on FireRowModel, which stores data
 * grouped by monitoring site. Best for site-specific operations.
 * 
 * Features:
 * - Serial and parallel execution modes
 * - OpenMP reductions for aggregations
 * - Efficient site-based queries
 * - Thread-local aggregation patterns
 */
class FireRowModelService : public IFireService {
private:
    const FireRowModel* model_;  ///< Pointer to the underlying data model

public:
    /// Constructor
    /// @param model Pointer to FireRowModel (must remain valid during service lifetime)
    explicit FireRowModelService(const FireRowModel* model);
    
    /// Destructor
    ~FireRowModelService() override;

    // IFireService implementation
    double averageConcentrationForParameter(const std::string& parameter, int numThreads = 1) const override;
    double sumConcentrationsForParameter(const std::string& parameter, int numThreads = 1) const override;
    double maxConcentrationForParameter(const std::string& parameter, int numThreads = 1) const override;
    double minConcentrationForParameter(const std::string& parameter, int numThreads = 1) const override;
    
    double averageAQI(int numThreads = 1) const override;
    int maxAQI(int numThreads = 1) const override;
    int minAQI(int numThreads = 1) const override;
    double averageAQIForParameter(const std::string& parameter, int numThreads = 1) const override;
    
    double averageConcentrationForSite(const std::string& siteName, int numThreads = 1) const override;
    std::size_t measurementCountForSite(const std::string& siteName) const override;
    
    std::size_t countMeasurementsInBounds(double minLat, double maxLat, 
                                          double minLon, double maxLon,
                                          int numThreads = 1) const override;
    double averageConcentrationInBounds(double minLat, double maxLat, 
                                        double minLon, double maxLon,
                                        int numThreads = 1) const override;
    
    std::vector<std::pair<std::string, double>> topNSitesByAverageConcentration(std::size_t n, int numThreads = 1) const override;
    std::vector<std::pair<std::string, int>> topNSitesByMaxAQI(std::size_t n, int numThreads = 1) const override;
    
    std::size_t countMeasurementsByCategory(int category, int numThreads = 1) const override;
    std::vector<std::size_t> categoryDistribution(int numThreads = 1) const override;
    
    std::string getImplementationName() const override;
    std::size_t totalMeasurementCount() const override;
    std::size_t uniqueSiteCount() const override;
};

/**
 * @class FireColumnModelService
 * @brief Fire analytics service using column-oriented data model
 * 
 * Implements analytics operations on FireColumnModel, which stores data
 * in columnar format. Best for analytics across many measurements.
 * 
 * Features:
 * - Serial and parallel execution modes
 * - OpenMP reductions for aggregations
 * - Cache-friendly column scanning
 * - Efficient parameter-based queries
 */
class FireColumnModelService : public IFireService {
private:
    const FireColumnModel* model_;  ///< Pointer to the underlying data model

public:
    /// Constructor
    /// @param model Pointer to FireColumnModel (must remain valid during service lifetime)
    explicit FireColumnModelService(const FireColumnModel* model);
    
    /// Destructor
    ~FireColumnModelService() override;

    // IFireService implementation
    double averageConcentrationForParameter(const std::string& parameter, int numThreads = 1) const override;
    double sumConcentrationsForParameter(const std::string& parameter, int numThreads = 1) const override;
    double maxConcentrationForParameter(const std::string& parameter, int numThreads = 1) const override;
    double minConcentrationForParameter(const std::string& parameter, int numThreads = 1) const override;
    
    double averageAQI(int numThreads = 1) const override;
    int maxAQI(int numThreads = 1) const override;
    int minAQI(int numThreads = 1) const override;
    double averageAQIForParameter(const std::string& parameter, int numThreads = 1) const override;
    
    double averageConcentrationForSite(const std::string& siteName, int numThreads = 1) const override;
    std::size_t measurementCountForSite(const std::string& siteName) const override;
    
    std::size_t countMeasurementsInBounds(double minLat, double maxLat, 
                                          double minLon, double maxLon,
                                          int numThreads = 1) const override;
    double averageConcentrationInBounds(double minLat, double maxLat, 
                                        double minLon, double maxLon,
                                        int numThreads = 1) const override;
    
    std::vector<std::pair<std::string, double>> topNSitesByAverageConcentration(std::size_t n, int numThreads = 1) const override;
    std::vector<std::pair<std::string, int>> topNSitesByMaxAQI(std::size_t n, int numThreads = 1) const override;
    
    std::size_t countMeasurementsByCategory(int category, int numThreads = 1) const override;
    std::vector<std::size_t> categoryDistribution(int numThreads = 1) const override;
    
    std::string getImplementationName() const override;
    std::size_t totalMeasurementCount() const override;
    std::size_t uniqueSiteCount() const override;
};


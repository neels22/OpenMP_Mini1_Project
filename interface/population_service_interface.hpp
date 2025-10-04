#pragma once

#include <vector>
#include <string>
#include <utility>

/**
 * @file population_service_interface.hpp
 * @brief Common interface for all population analytics services
 * 
 * This interface defines the contract that all population analytics services must implement,
 * enabling polymorphic usage and eliminating code duplication in benchmarks and client code.
 * Both row-oriented and column-oriented services implement this interface.
 */

/**
 * @interface IPopulationService
 * @brief Abstract interface for population analytics operations
 * 
 * This interface provides a common contract for population analytics services,
 * enabling clean separation between business logic and data layout implementations.
 * All implementations must support both serial and parallel execution modes.
 * 
 * Key Benefits:
 * - Eliminates duplicate benchmark code
 * - Enables polymorphic service usage
 * - Enforces consistent API across implementations
 * - Facilitates testing and mocking
 */
class IPopulationService {
public:
    virtual ~IPopulationService() = default;

    // === Aggregation Operations ===
    
    /// Calculate total population across all countries for a specific year
    /// @param year The target year for calculation
    /// @param numThreads Number of threads for parallel execution (1 = serial)
    /// @return Total population or 0 if year not found
    virtual long long sumPopulationForYear(int year, int numThreads = 1) const = 0;
    
    /// Calculate average population across all countries for a specific year
    /// @param year The target year for calculation  
    /// @param numThreads Number of threads for parallel execution (1 = serial)
    /// @return Average population or 0.0 if year not found
    virtual double averagePopulationForYear(int year, int numThreads = 1) const = 0;
    
    /// Find maximum population among all countries for a specific year
    /// @param year The target year for calculation
    /// @param numThreads Number of threads for parallel execution (1 = serial)
    /// @return Maximum population or 0 if year not found
    virtual long long maxPopulationForYear(int year, int numThreads = 1) const = 0;
    
    /// Find minimum population among all countries for a specific year
    /// @param year The target year for calculation
    /// @param numThreads Number of threads for parallel execution (1 = serial)
    /// @return Minimum population or 0 if year not found
    virtual long long minPopulationForYear(int year, int numThreads = 1) const = 0;

    // === Country-Specific Operations ===
    
    /// Get population for a specific country in a specific year
    /// @param country Country name to look up
    /// @param year Target year
    /// @param numThreads Number of threads (typically unused for single lookups)
    /// @return Population value or 0 if country/year not found
    virtual long long populationForCountryInYear(const std::string& country, int year, int numThreads = 1) const = 0;
    
    /// Get population data for a specific country across a range of years
    /// @param country Country name to look up
    /// @param startYear Start of year range (inclusive)
    /// @param endYear End of year range (inclusive)
    /// @param numThreads Number of threads for parallel execution
    /// @return Vector of population values indexed by (year - startYear)
    virtual std::vector<long long> populationOverYearsForCountry(const std::string& country, int startYear, int endYear, int numThreads = 1) const = 0;

    // === Top-N Operations ===
    
    /// Find top N countries by population for a specific year
    /// @param year Target year for ranking
    /// @param n Number of top countries to return
    /// @param numThreads Number of threads for parallel execution
    /// @return Vector of (country_name, population) pairs sorted by population (descending)
    virtual std::vector<std::pair<std::string, long long>> topNCountriesByPopulationInYear(int year, std::size_t n, int numThreads = 1) const = 0;
    
    // === Metadata Operations ===
    
    /// Get a human-readable name for this service implementation
    /// @return Implementation name (e.g., "Row-oriented", "Column-oriented")
    virtual std::string getImplementationName() const = 0;
};
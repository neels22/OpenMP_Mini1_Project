#pragma once

#include <string>
#include <vector>
#include <unordered_map>

/**
 * @file populationModelColumn.hpp
 * @brief Column-oriented population data model for efficient per-year operations
 * 
 * This file defines the column-oriented data model where each year's data is stored
 * as a contiguous vector of population values across all countries. This layout is
 * optimal for analytics workloads that need to aggregate or analyze data across
 * many countries for specific years.
 */

/**
 * @class PopulationModelColumn
 * @brief Column-oriented population data model for efficient year-based aggregations
 * 
 * This model stores data as vectors of columns, where each column contains all
 * countries' population data for one specific year. This layout provides excellent
 * cache locality and vectorization opportunities for:
 * - Per-year aggregations (sum, average, min, max across all countries)
 * - Analytics queries that scan many countries for specific years
 * - Parallel reductions that can process contiguous memory efficiently
 * 
 * Data Layout:
 * ```
 * Year 2000: [Country1_Pop, Country2_Pop, ..., CountryN_Pop]
 * Year 2001: [Country1_Pop, Country2_Pop, ..., CountryN_Pop]
 * ...
 * Year 202X: [Country1_Pop, Country2_Pop, ..., CountryN_Pop]
 * ```
 * 
 * Trade-offs:
 * + Excellent for per-year aggregations and analytics
 * + Superior cache locality for operations across many countries
 * + Better vectorization and parallel reduction performance
 * + Direct indexing for country-year lookups (O(1) access)
 * - Less efficient for per-country time series operations
 * - Requires year-country coordinate calculations
 */
class PopulationModelColumn {
private:
    // Metadata vectors (parallel arrays for fast indexed access)
    std::vector<std::string> _countryNames;         ///< Country names in insertion order
    std::vector<std::string> _countriesCode;        ///< Country codes in insertion order  
    std::vector<std::string> _indicatorNames;       ///< Indicator names (usually "Population")
    std::vector<std::string> _indicatorCodes;       ///< Indicator codes (usually population codes)
    std::vector<long long> _years;                  ///< Year values in column order

    /**
     * Core data storage: columnar layout
     * _columns[year_index][country_index] = population_value
     * 
     * Each inner vector contains all countries' populations for one year,
     * providing excellent cache locality for per-year operations.
     */
    std::vector<std::vector<long long>> _columns;

    // Fast lookup indices for O(1) access
    std::unordered_map<std::string, int> _countryNameToIndex;           ///< Country name -> index
    std::unordered_map<std::string, std::string> _countryNameToCountryCode; ///< Name -> code mapping
    std::unordered_map<long long, int> _yearToIndex;                    ///< Year -> column index

public:
    /// Default constructor - initializes empty model
    PopulationModelColumn();
    
    /// Destructor - default cleanup is sufficient
    ~PopulationModelColumn();

    // === Metadata Access Methods ===
    
    /// Get all country names in insertion order
    const std::vector<std::string>& countryNames() const noexcept;
    
    /// Get all country codes in insertion order
    const std::vector<std::string>& countriesCode() const noexcept;
    
    /// Get all indicator names (typically population indicators)
    const std::vector<std::string>& indicatorNames() const noexcept;
    
    /// Get all indicator codes (typically population codes)
    const std::vector<std::string>& indicatorCodes() const noexcept;
    
    /// Get all year values in column order
    const std::vector<long long>& years() const noexcept;

    /// Get country name to index mapping (for advanced use)
    const std::unordered_map<std::string, int>& countryNameToIndex() const noexcept;
    
    /// Get year to column index mapping (for advanced use)
    const std::unordered_map<long long, int>& yearToIndex() const noexcept;

    // === Size Information Methods ===
    
    /// Get total number of countries in the model
    std::size_t rowCount() const noexcept;
    
    /// Get total number of years in the model
    std::size_t yearCount() const noexcept;

    // === Data Modification Methods ===
    
    /// Insert a new country's data (appends to existing data)
    /// Accepts same parameters as row model for API compatibility
    void insertNewEntry(std::string country, std::string country_code, std::string indicator_name, std::string indicator_code, std::vector<long long> year_population);

    /// Set the years vector (must be called before inserting any country data)
    bool setYears(std::vector<long long> years);

    /// Load data from CSV file with comprehensive error handling
    /// Expects same CSV format as row model: Country Name, Country Code, Indicator Name, Indicator Code, <year columns...>
    void readFromCSV(const std::string& filename);

    // === Data Access Methods ===
    
    /// Get population value by country index and year index (direct O(1) access)
    /// This is the fastest way to access data in the columnar model
    long long getPopulationForCountryYear(std::size_t countryIndex, std::size_t yearIndex) const;

    /// Find country index by name. Returns -1 if not found
    int countryNameIndex(const std::string& country) const noexcept;
};

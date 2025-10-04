#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <unordered_map>

/**
 * @file populationModel.hpp
 * @brief Row-oriented population data model for efficient per-country operations
 * 
 * This file defines the row-oriented data model where each country's data is stored
 * as a contiguous vector of population values across years. This layout is optimal
 * for operations that need to access all years for a specific country.
 */

/**
 * @class PopulationRow
 * @brief Represents one country's population values across a sequence of years
 * 
 * Each PopulationRow contains:
 * - Country name (string identifier)
 * - Vector of population values indexed by year position
 * 
 * This design provides O(1) access to population data for a specific country-year
 * combination once the row is located.
 */
class PopulationRow {
private:
    std::string _country;                    ///< Country name identifier
    std::vector<long long> _year_population; ///< Population values by year index

public:
    /// Default constructor - creates empty row
    PopulationRow();
    
    /// Parameterized constructor with move semantics for efficiency
    PopulationRow(std::string country, std::vector<long long> year_population);

    // Getters
    /// Get country name (const reference to avoid copying)
    const std::string& country() const noexcept;
    
    /// Get all population data (const reference to avoid copying)
    const std::vector<long long>& yearPopulation() const noexcept;
    
    /// Get population for specific year index (bounds checking in implementation)
    long long getPopulationForYear(std::size_t yearIndex) const;
    
    /// Get number of years with data
    std::size_t yearCount() const noexcept;
};

/**
 * @class PopulationModel
 * @brief Row-oriented population data model for efficient country-based queries
 * 
 * This model stores data as a vector of PopulationRow objects, where each row
 * contains all population data for one country across all years. This layout
 * provides excellent cache locality for:
 * - Per-country operations (getting all years for one country)
 * - Sequential access patterns within a country's data
 * 
 * Trade-offs:
 * + Excellent for country-specific queries and operations
 * + Good cache locality for per-country time series analysis
 * - Less efficient for per-year aggregations across all countries
 * - Requires country name lookup for random access
 */
class PopulationModel {
private:
    // Core data storage
    std::vector<PopulationRow> _rows;               ///< Main data storage: one row per country
    
    // Metadata vectors (parallel arrays for fast indexed access)
    std::vector<std::string> _countryNames;         ///< Country names in row order
    std::vector<std::string> _countriesCode;        ///< Country codes in row order
    std::vector<std::string> _indicatorNames;       ///< Indicator names (usually "Population")
    std::vector<std::string> _indicatorCodes;       ///< Indicator codes (usually population codes)
    std::vector<long long> _years;                  ///< Year values in column order
    
    // Fast lookup indices for O(1) access
    std::unordered_map<std::string, int> _countryCodeToRowIndex;    ///< Country code -> row index
    std::unordered_map<long long, int> _yearToIndex;                ///< Year -> column index
    std::unordered_map<std::string,std::string> _countryNameToCountryCode; ///< Name -> code mapping

public:
    /// Default constructor - initializes empty model
    PopulationModel();
    
    /// Destructor - default cleanup is sufficient
    ~PopulationModel();

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

    /// Get country name to row index mapping (for advanced use)
    const std::unordered_map<std::string, int>& countryNameToIndex() const noexcept;
    
    /// Get year to column index mapping (for advanced use)
    const std::unordered_map<long long, int>& yearToIndex() const noexcept;

    // === Data Access Methods ===
    
    /// Get total number of countries (rows) in the model
    std::size_t rowCount() const noexcept;
    
    /// Get specific country's data by row index (bounds checking in implementation)
    const PopulationRow& rowAt(std::size_t idx) const;

    /// Find a country's row by name. Returns nullptr if not found
    const PopulationRow* getByCountry(const std::string& country) const noexcept;

    // === Data Modification Methods ===
    
    /// Set the years vector (only allowed if no data rows exist yet)
    bool setYears(std::vector<long long> years);
    
    /// Load data from CSV file with comprehensive error handling
    void readFromCSV(const std::string& filename);
    
    /// Insert a new country's data (appends to existing data)
    void insertNewEntry(std::string country, std::string contry_code, std::string indicator_name, std::string indicator_code, std::vector<long long> year_population);
};

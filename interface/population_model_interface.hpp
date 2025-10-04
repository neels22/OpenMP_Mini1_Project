#pragma once

#include <vector>
#include <string>
#include <unordered_map>

/**
 * @file population_model_interface.hpp
 * @brief Common interface for population data models
 * 
 * This interface defines the contract that all population data models must implement,
 * enabling polymorphic usage and eliminating code duplication in client code.
 * Both row-oriented and column-oriented models implement this interface.
 */

/**
 * @interface IPopulationModel
 * @brief Abstract interface for population data models
 * 
 * This interface provides a common contract for population data models,
 * enabling clean separation between data layout implementation and client code.
 * All implementations must provide access to country names, years, and population data.
 * 
 * Key Benefits:
 * - Enables polymorphic model usage
 * - Enforces consistent API across implementations
 * - Facilitates testing and mocking
 * - Reduces coupling between data layout and business logic
 */
class IPopulationModel {
public:
    virtual ~IPopulationModel() = default;

    // === Metadata Access ===
    
    /// Get list of all available years in the dataset
    /// @return Vector of years in chronological order
    virtual const std::vector<long long>& years() const = 0;
    
    /// Get total number of countries in the dataset
    /// @return Number of country records
    virtual std::size_t rowCount() const = 0;
    
    /// Get mapping from year to column index
    /// @return Map of year -> index for efficient year lookups
    virtual const std::unordered_map<long long, int>& yearToIndex() const = 0;
    
    /// Get mapping from country name to row index
    /// @return Map of country_name -> index for efficient country lookups
    virtual const std::unordered_map<std::string, int>& countryNameToIndex() const = 0;

    // === Data Access ===
    
    /// Get population value for specific country and year indices
    /// @param countryIndex Zero-based country index
    /// @param yearIndex Zero-based year index  
    /// @return Population value or 0 if indices are invalid
    virtual long long getPopulationAt(std::size_t countryIndex, std::size_t yearIndex) const = 0;
    
    /// Get country name by index
    /// @param countryIndex Zero-based country index
    /// @return Country name or empty string if index is invalid
    virtual std::string getCountryNameAt(std::size_t countryIndex) const = 0;
    
    // === Utility Methods ===
    
    /// Get human-readable name for this model implementation
    /// @return Implementation name (e.g., "Row-oriented", "Column-oriented")
    virtual std::string getModelType() const = 0;
    
    /// Validate model consistency and data integrity
    /// @return True if model is valid and consistent
    virtual bool validate() const = 0;
};
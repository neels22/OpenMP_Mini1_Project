#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <unordered_map>

/**
 * @file fireRowModel.hpp
 * @brief Row-oriented fire air quality data model for efficient site-based operations
 * 
 * This file defines the row-oriented data model where each monitoring site's data is stored
 * as a contiguous vector of air quality measurements across time. This layout is optimal
 * for operations that need to access all measurements for a specific site.
 */

/**
 * @class FireMeasurement
 * @brief Represents a single air quality measurement at a specific time and location
 * 
 * Each FireMeasurement contains all the data from one CSV row:
 * - Location (latitude, longitude)
 * - Time information
 * - Measurement details (parameter, concentration, AQI, etc.)
 * - Site and agency information
 */
class FireMeasurement {
private:
    double _latitude;                ///< Latitude coordinate
    double _longitude;               ///< Longitude coordinate
    std::string _datetime;           ///< DateTime string (ISO format)
    std::string _parameter;          ///< Parameter type (PM2.5, PM10, etc.)
    double _concentration;           ///< Measured concentration value
    std::string _unit;               ///< Unit of measurement (UG/M3, etc.)
    double _raw_concentration;       ///< Raw concentration value
    int _aqi;                        ///< Air Quality Index
    int _category;                   ///< AQI category
    std::string _site_name;          ///< Monitoring site name
    std::string _agency_name;        ///< Responsible agency name
    std::string _aqs_code;           ///< AQS code (short)
    std::string _full_aqs_code;      ///< Full AQS code

public:
    /// Default constructor
    FireMeasurement();
    
    /// Parameterized constructor
    FireMeasurement(double latitude, double longitude, std::string datetime,
                   std::string parameter, double concentration, std::string unit,
                   double raw_concentration, int aqi, int category,
                   std::string site_name, std::string agency_name,
                   std::string aqs_code, std::string full_aqs_code);

    // Getters
    double latitude() const noexcept;
    double longitude() const noexcept;
    const std::string& datetime() const noexcept;
    const std::string& parameter() const noexcept;
    double concentration() const noexcept;
    const std::string& unit() const noexcept;
    double rawConcentration() const noexcept;
    int aqi() const noexcept;
    int category() const noexcept;
    const std::string& siteName() const noexcept;
    const std::string& agencyName() const noexcept;
    const std::string& aqsCode() const noexcept;
    const std::string& fullAqsCode() const noexcept;
};

/**
 * @class FireSiteData
 * @brief Represents all measurements for a specific monitoring site
 * 
 * Each FireSiteData contains:
 * - Site identifier (name or code)
 * - Vector of all measurements taken at this site
 * 
 * This design provides efficient access to all measurements for a specific site.
 */
class FireSiteData {
private:
    std::string _site_identifier;                    ///< Site identifier (name or AQS code)
    std::vector<FireMeasurement> _measurements;      ///< All measurements for this site

public:
    /// Default constructor
    FireSiteData();
    
    /// Parameterized constructor
    FireSiteData(std::string site_identifier, std::vector<FireMeasurement> measurements);

    // Getters
    const std::string& siteIdentifier() const noexcept;
    const std::vector<FireMeasurement>& measurements() const noexcept;
    
    /// Get specific measurement by index
    const FireMeasurement& getMeasurement(std::size_t index) const;
    
    /// Get number of measurements for this site
    std::size_t measurementCount() const noexcept;
    
    /// Add a new measurement to this site
    void addMeasurement(const FireMeasurement& measurement);
};

/**
 * @class FireRowModel
 * @brief Row-oriented fire air quality data model for efficient site-based queries
 * 
 * This model stores data as a vector of FireSiteData objects, where each entry
 * contains all measurements for one monitoring site across all times. This layout
 * provides excellent cache locality for:
 * - Per-site operations (getting all measurements for one site)
 * - Time series analysis for specific monitoring locations
 * 
 * Trade-offs:
 * + Excellent for site-specific queries and operations
 * + Good cache locality for per-site time series analysis
 * + Efficient for filtering by site characteristics
 * - Less efficient for temporal aggregations across all sites
 * - Requires site lookup for random access by location
 */
class FireRowModel {
private:
    // Core data storage
    std::vector<FireSiteData> _sites;                           ///< Main data storage: one entry per site
    
    // Metadata for fast access
    std::vector<std::string> _site_names;                       ///< All unique site names
    std::vector<std::string> _parameters;                       ///< All unique parameters (PM2.5, PM10, etc.)
    std::vector<std::string> _agencies;                         ///< All unique agency names
    std::vector<std::string> _datetime_range;                   ///< Date/time range [start, end]
    
    // Fast lookup indices
    std::unordered_map<std::string, int> _site_name_to_index;   ///< Site name -> index mapping
    std::unordered_map<std::string, int> _aqs_code_to_index;    ///< AQS code -> index mapping
    
    // Statistics for quick access
    std::size_t _total_measurements;                            ///< Total number of measurements
    double _min_latitude, _max_latitude;                        ///< Latitude bounds
    double _min_longitude, _max_longitude;                      ///< Longitude bounds

public:
    /// Default constructor
    FireRowModel();
    
    /// Destructor
    ~FireRowModel();

    // === Metadata Access Methods ===
    
    /// Get all unique site names
    const std::vector<std::string>& siteNames() const noexcept;
    
    /// Get all unique parameters
    const std::vector<std::string>& parameters() const noexcept;
    
    /// Get all unique agencies
    const std::vector<std::string>& agencies() const noexcept;
    
    /// Get datetime range [start, end]
    const std::vector<std::string>& datetimeRange() const noexcept;
    
    /// Get site name to index mapping
    const std::unordered_map<std::string, int>& siteNameToIndex() const noexcept;

    // === Data Access Methods ===
    
    /// Get total number of sites
    std::size_t siteCount() const noexcept;
    
    /// Get total number of measurements across all sites
    std::size_t totalMeasurements() const noexcept;
    
    /// Get specific site's data by index
    const FireSiteData& siteAt(std::size_t idx) const;
    
    /// Find site data by name. Returns nullptr if not found
    const FireSiteData* getBySiteName(const std::string& site_name) const noexcept;
    
    /// Find site data by AQS code. Returns nullptr if not found
    const FireSiteData* getByAqsCode(const std::string& aqs_code) const noexcept;
    
    /// Get geographic bounds
    void getGeographicBounds(double& min_lat, double& max_lat, 
                           double& min_lon, double& max_lon) const noexcept;

    // === Data Modification Methods ===
    
    /// Load data from CSV file with comprehensive error handling
    void readFromCSV(const std::string& filename);
    
    /// Load data from multiple CSV files (for processing multiple dates/times)
    void readFromMultipleCSV(const std::vector<std::string>& filenames);
    
    /// Load data from multiple CSV files with OpenMP parallelization and dynamic load balancing
    /// @param filenames Vector of CSV file paths to process
    /// @param num_threads Number of threads to use (if <= 1, uses single thread)
    void readFromMultipleCSVParallel(const std::vector<std::string>& filenames, int num_threads = 3);
    
    /// Load all CSV files from a directory
    void readFromDirectory(const std::string& directory_path);
    
    /// Load all CSV files from a directory with parallel processing
    /// @param directory_path Path to directory containing CSV files
    /// @param num_threads Number of threads to use (if <= 1, uses single thread)
    void readFromDirectoryParallel(const std::string& directory_path, int num_threads = 3);
    
    /// Insert a new measurement (creates new site if needed)
    void insertMeasurement(const FireMeasurement& measurement);
    
    /// Clear all data
    void clear();

private:
    /// Helper method to update metadata when adding measurements
    void updateMetadata(const FireMeasurement& measurement);
    
    /// Helper method to parse a vector of CSV tokens into a FireMeasurement
    FireMeasurement parseCSVRow(const std::vector<std::string>& tokens) const;
    
    /// Helper method to find or create site index
    int findOrCreateSiteIndex(const std::string& site_name, const std::string& aqs_code);
    
    /// Helper method to merge data from another FireRowModel instance
    void mergeFromModel(const FireRowModel& other);
};

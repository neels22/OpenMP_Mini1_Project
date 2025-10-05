#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

/**
 * @file fireColumnModel.hpp
 * @brief Column-oriented fire air quality data model for efficient analytics operations
 * 
 * This file defines the column-oriented data model where each field (latitude, longitude,
 * concentration, etc.) is stored as a contiguous vector across all measurements. This layout
 * is optimal for analytics workloads that need to aggregate or analyze data across many
 * measurements for specific parameters, locations, or time ranges.
 */

/**
 * @class FireColumnModel
 * @brief Column-oriented fire air quality data model for efficient analytics
 * 
 * This model stores data as vectors of columns, where each column contains all
 * measurements' data for one specific field. This layout provides excellent
 * cache locality and vectorization opportunities for:
 * - Per-parameter aggregations (average PM2.5, max AQI, etc.)
 * - Analytics queries that scan many measurements for specific criteria
 * - Parallel reductions that can process contiguous memory efficiently
 * - Geographic and temporal analytics across large datasets
 * 
 * Data Layout:
 * ```
 * Latitudes:      [Meas1_Lat,  Meas2_Lat,  ..., MeasN_Lat]
 * Longitudes:     [Meas1_Lon,  Meas2_Lon,  ..., MeasN_Lon]
 * Concentrations: [Meas1_Conc, Meas2_Conc, ..., MeasN_Conc]
 * ...
 * ```
 * 
 * Trade-offs:
 * + Excellent for analytics across many measurements
 * + Superior cache locality for field-specific operations
 * + Better vectorization and parallel reduction performance
 * + Efficient for statistical analysis and aggregations
 * - Requires coordination between parallel vectors for complete measurement access
 * - More complex insertion logic compared to row model
 */
class FireColumnModel {
private:
    // Columnar storage - each vector contains all measurements' values for one field
    std::vector<double> _latitudes;              ///< All measurement latitudes
    std::vector<double> _longitudes;             ///< All measurement longitudes
    std::vector<std::string> _datetimes;         ///< All measurement datetimes
    std::vector<std::string> _parameters;        ///< All measurement parameters (PM2.5, PM10, etc.)
    std::vector<double> _concentrations;         ///< All measured concentration values
    std::vector<std::string> _units;             ///< All measurement units
    std::vector<double> _raw_concentrations;     ///< All raw concentration values
    std::vector<int> _aqis;                      ///< All Air Quality Index values
    std::vector<int> _categories;                ///< All AQI categories
    std::vector<std::string> _site_names;        ///< All monitoring site names
    std::vector<std::string> _agency_names;      ///< All responsible agency names
    std::vector<std::string> _aqs_codes;         ///< All AQS codes (short)
    std::vector<std::string> _full_aqs_codes;    ///< All full AQS codes

    // Index structures for fast lookups
    std::unordered_map<std::string, std::vector<std::size_t>> _site_indices;      ///< Site name -> measurement indices
    std::unordered_map<std::string, std::vector<std::size_t>> _parameter_indices; ///< Parameter -> measurement indices
    std::unordered_map<std::string, std::vector<std::size_t>> _aqs_indices;       ///< AQS code -> measurement indices
    
    // Metadata tracking
    std::unordered_set<std::string> _unique_sites;      ///< All unique site names
    std::unordered_set<std::string> _unique_parameters; ///< All unique parameters
    std::unordered_set<std::string> _unique_agencies;   ///< All unique agencies
    std::vector<std::string> _datetime_range;           ///< [min_datetime, max_datetime]
    
    // Geographic bounds tracking
    double _min_latitude, _max_latitude;
    double _min_longitude, _max_longitude;
    bool _bounds_initialized;

public:
    /// Default constructor
    FireColumnModel();
    
    /// Destructor
    ~FireColumnModel();

    // === Data Loading Methods ===
    
    /**
     * @brief Read fire data from all CSV files in a directory (parallel or serial)
     * @param directoryPath Path to directory containing CSV files
     * @param numThreads Number of threads to use (if <= 1, uses serial processing)
     * 
     * This is the main entry point for loading fire data. It automatically
     * discovers all CSV files in the directory and processes them using
     * either serial or parallel approach based on numThreads parameter.
     */
    void readFromDirectory(const std::string& directoryPath, int numThreads = 1);
    
    /**
     * @brief Read fire data from all CSV files in a directory (parallel version)
     * @param directoryPath Path to directory containing CSV files
     * @param numThreads Number of threads to use for parallel processing
     * 
     * Uses OpenMP to distribute CSV files across multiple threads for faster
     * ingestion. Each thread processes a subset of files into a local model,
     * then all local models are merged into this instance.
     */
    void readFromDirectoryParallel(const std::string& directoryPath, int numThreads);

    /**
     * @brief Read fire data from a single CSV file
     * @param filename Path to CSV file to read
     * 
     * Processes one CSV file and adds all measurements to the columnar storage.
     * Used by both serial and parallel ingestion methods.
     */
    void readFromCSV(const std::string& filename);

    /**
     * @brief Insert a single measurement into the columnar storage
     * @param latitude Measurement latitude
     * @param longitude Measurement longitude
     * @param datetime Measurement datetime string
     * @param parameter Parameter type (PM2.5, PM10, etc.)
     * @param concentration Measured concentration value
     * @param unit Unit of measurement
     * @param raw_concentration Raw concentration value
     * @param aqi Air Quality Index
     * @param category AQI category
     * @param site_name Monitoring site name
     * @param agency_name Responsible agency name
     * @param aqs_code AQS code (short)
     * @param full_aqs_code Full AQS code
     */
    void insertMeasurement(double latitude, double longitude, const std::string& datetime,
                          const std::string& parameter, double concentration, const std::string& unit,
                          double raw_concentration, int aqi, int category,
                          const std::string& site_name, const std::string& agency_name,
                          const std::string& aqs_code, const std::string& full_aqs_code);

    /**
     * @brief Merge another FireColumnModel into this one
     * @param other The model to merge from
     * 
     * Used during parallel processing to combine thread-local models.
     * Efficiently appends all data from the other model and updates indices.
     */
    void mergeFromModel(const FireColumnModel& other);

    // === Query Methods ===
    
    /**
     * @brief Get indices of all measurements for a specific site
     * @param siteName Name of the monitoring site
     * @return Vector of measurement indices for the site
     */
    std::vector<std::size_t> getIndicesBySite(const std::string& siteName) const;
    
    /**
     * @brief Get indices of all measurements for a specific parameter
     * @param parameter Parameter type (PM2.5, PM10, etc.)
     * @return Vector of measurement indices for the parameter
     */
    std::vector<std::size_t> getIndicesByParameter(const std::string& parameter) const;
    
    /**
     * @brief Get indices of all measurements for a specific AQS code
     * @param aqsCode AQS code to search for
     * @return Vector of measurement indices for the AQS code
     */
    std::vector<std::size_t> getIndicesByAqsCode(const std::string& aqsCode) const;

    // === Accessors for Columnar Data ===
    
    const std::vector<double>& latitudes() const noexcept { return _latitudes; }
    const std::vector<double>& longitudes() const noexcept { return _longitudes; }
    const std::vector<std::string>& datetimes() const noexcept { return _datetimes; }
    const std::vector<std::string>& parameters() const noexcept { return _parameters; }
    const std::vector<double>& concentrations() const noexcept { return _concentrations; }
    const std::vector<std::string>& units() const noexcept { return _units; }
    const std::vector<double>& rawConcentrations() const noexcept { return _raw_concentrations; }
    const std::vector<int>& aqis() const noexcept { return _aqis; }
    const std::vector<int>& categories() const noexcept { return _categories; }
    const std::vector<std::string>& siteNames() const noexcept { return _site_names; }
    const std::vector<std::string>& agencyNames() const noexcept { return _agency_names; }
    const std::vector<std::string>& aqsCodes() const noexcept { return _aqs_codes; }
    const std::vector<std::string>& fullAqsCodes() const noexcept { return _full_aqs_codes; }

    // === Metadata and Statistics ===
    
    /**
     * @brief Get total number of measurements
     * @return Number of measurements stored
     */
    std::size_t measurementCount() const noexcept { return _latitudes.size(); }
    
    /**
     * @brief Get number of unique monitoring sites
     * @return Number of unique sites
     */
    std::size_t siteCount() const noexcept { return _unique_sites.size(); }
    
    /**
     * @brief Get all unique monitoring sites
     * @return Set of unique site names
     */
    const std::unordered_set<std::string>& uniqueSites() const noexcept { return _unique_sites; }
    
    /**
     * @brief Get all unique parameters measured
     * @return Set of unique parameter types
     */
    const std::unordered_set<std::string>& uniqueParameters() const noexcept { return _unique_parameters; }
    
    /**
     * @brief Get all unique agencies
     * @return Set of unique agency names
     */
    const std::unordered_set<std::string>& uniqueAgencies() const noexcept { return _unique_agencies; }
    
    /**
     * @brief Get datetime range of all measurements
     * @return Vector with [min_datetime, max_datetime]
     */
    const std::vector<std::string>& datetimeRange() const noexcept { return _datetime_range; }
    
    /**
     * @brief Get geographic bounds of all measurements
     * @param min_lat Output for minimum latitude
     * @param max_lat Output for maximum latitude
     * @param min_lon Output for minimum longitude
     * @param max_lon Output for maximum longitude
     */
    void getGeographicBounds(double& min_lat, double& max_lat, 
                            double& min_lon, double& max_lon) const;

private:
    /**
     * @brief Update indices after inserting a new measurement
     * @param index Index of the newly inserted measurement
     */
    void updateIndices(std::size_t index);
    
    /**
     * @brief Update geographic bounds with new coordinates
     * @param latitude New latitude value
     * @param longitude New longitude value
     */
    void updateGeographicBounds(double latitude, double longitude);
    
    /**
     * @brief Update datetime range with new datetime
     * @param datetime New datetime string
     */
    void updateDatetimeRange(const std::string& datetime);
    
    /**
     * @brief Get list of all CSV files in a directory
     * @param directoryPath Path to directory to scan
     * @return Vector of CSV file paths
     */
    std::vector<std::string> getCSVFiles(const std::string& directoryPath) const;
};

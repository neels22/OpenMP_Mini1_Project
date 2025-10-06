#pragma once

#include <string>
#include <vector>
#include <cmath>

/**
 * @file airquality_types.hpp
 * @brief Core data structures for air quality monitoring data
 * 
 * This file defines the fundamental data types for air quality analysis,
 * including individual measurement records and station metadata.
 */

namespace AirQuality {

/**
 * @struct Record
 * @brief Single air quality measurement record
 * 
 * Represents one measurement from one monitoring station at a specific time.
 * Contains geographic location, temporal information, pollutant details,
 * and quality metadata.
 */
struct Record {
    // Geographic coordinates
    double latitude;
    double longitude;
    
    // Temporal information
    long long timestamp;        ///< Unix timestamp (seconds since epoch) for fast comparison
    std::string dateTimeStr;    ///< Original ISO 8601 datetime string (e.g., "2020-08-10T01:00")
    
    // Measurement information
    std::string pollutant;      ///< Pollutant type (e.g., "PM2.5", "PM10", "OZONE")
    double value;               ///< Measured pollutant value
    std::string unit;           ///< Measurement unit (e.g., "UG/M3", "PPB")
    
    // Air Quality Index
    double aqi;                 ///< Air Quality Index value
    int aqiCategory;            ///< AQI category/risk level
    int qualityFlag;            ///< Data quality indicator
    
    // Station metadata
    std::string location;       ///< Station/location name
    std::string agency;         ///< Monitoring agency name
    std::string siteId1;        ///< Primary site identifier
    std::string siteId2;        ///< Secondary site identifier
    
    /**
     * @brief Default constructor - creates empty/invalid record
     */
    Record() 
        : latitude(0.0), longitude(0.0), timestamp(0), 
          value(0.0), aqi(0.0), aqiCategory(0), qualityFlag(0) {}
    
    /**
     * @brief Check if this record contains valid data
     * @return true if record appears valid, false otherwise
     * 
     * Validates essential fields to ensure record is usable.
     * Checks for reasonable geographic coordinates, non-empty identifiers,
     * and valid timestamp.
     */
    bool isValid() const {
        // Check latitude/longitude are in valid ranges
        if (latitude < -90.0 || latitude > 90.0) return false;
        if (longitude < -180.0 || longitude > 180.0) return false;
        
        // Check essential fields are non-empty
        if (siteId1.empty() || pollutant.empty()) return false;
        
        // Check timestamp is reasonable (after 2000-01-01 and before 2100-01-01)
        if (timestamp < 946684800 || timestamp > 4102444800) return false;
        
        // Check value is finite (not NaN or infinity)
        if (!std::isfinite(value)) return false;
        
        return true;
    }
    
    /**
     * @brief Convert record to human-readable string
     * @return String representation of this record
     * 
     * Useful for debugging and logging. Formats key fields in readable format.
     */
    std::string toString() const {
        return "[" + dateTimeStr + "] " + location + " (" + siteId1 + "): " 
               + pollutant + "=" + std::to_string(value) + " " + unit;
    }
};

/**
 * @struct StationInfo
 * @brief Metadata about a monitoring station
 * 
 * Aggregated information about a station for quick lookup and analysis.
 * Used by both row and column models to maintain station reference data.
 */
struct StationInfo {
    std::string siteId;         ///< Unique station identifier
    std::string location;       ///< Human-readable station name
    double latitude;            ///< Station latitude
    double longitude;           ///< Station longitude
    std::string agency;         ///< Operating agency
    size_t recordCount;         ///< Number of records for this station
    
    /**
     * @brief Default constructor
     */
    StationInfo() 
        : latitude(0.0), longitude(0.0), recordCount(0) {}
    
    /**
     * @brief Calculate distance to another point using Haversine formula
     * @param lat Target latitude
     * @param lon Target longitude
     * @return Distance in kilometers
     * 
     * Calculates great-circle distance between this station and another point
     * on Earth. Useful for spatial queries like "find stations within radius".
     */
    double distanceTo(double lat, double lon) const {
        // Haversine formula
        const double R = 6371.0; // Earth radius in km
        
        double lat1 = latitude * M_PI / 180.0;
        double lat2 = lat * M_PI / 180.0;
        double dLat = (lat - latitude) * M_PI / 180.0;
        double dLon = (lon - longitude) * M_PI / 180.0;
        
        double a = std::sin(dLat/2) * std::sin(dLat/2) +
                   std::cos(lat1) * std::cos(lat2) *
                   std::sin(dLon/2) * std::sin(dLon/2);
        
        double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
        
        return R * c;
    }
    
    /**
     * @brief Check if station is within a bounding box
     * @param minLat Minimum latitude
     * @param maxLat Maximum latitude
     * @param minLon Minimum longitude
     * @param maxLon Maximum longitude
     * @return true if station is within bounds
     */
    bool isInBoundingBox(double minLat, double maxLat, 
                        double minLon, double maxLon) const {
        return latitude >= minLat && latitude <= maxLat &&
               longitude >= minLon && longitude <= maxLon;
    }
};

/**
 * @struct FileLoadResult
 * @brief Result from loading a single CSV file
 * 
 * Contains loaded records, performance metrics, and error information.
 * Used by parallel file loader to track results from each thread.
 */
struct FileLoadResult {
    std::string filename;           ///< Path to loaded file
    std::vector<Record> records;    ///< Loaded records
    size_t recordCount;             ///< Number of successfully loaded records
    double loadTimeMs;              ///< Load time in milliseconds
    bool success;                   ///< Whether load was successful
    std::string errorMsg;           ///< Error message if load failed
    
    /**
     * @brief Default constructor
     */
    FileLoadResult() 
        : recordCount(0), loadTimeMs(0.0), success(false) {}
};

} // namespace AirQuality


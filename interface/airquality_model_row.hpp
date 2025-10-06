#pragma once

#include "airquality_types.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>

/**
 * @file airquality_model_row.hpp
 * @brief Row-oriented (station-centric) air quality data model
 * 
 * This model organizes data by monitoring station, where each station contains
 * all its measurements in chronological order. This layout is optimal for:
 * - Time-series queries for specific stations
 * - Station-specific analysis
 * - Sequential temporal access patterns
 * 
 * Trade-offs:
 * + Excellent for station-specific queries
 * + Good cache locality for per-station analysis
 * - Less efficient for cross-station temporal aggregations
 * - Requires scanning all stations for time-slice queries
 */

namespace AirQuality {

/**
 * @class RowModel
 * @brief Station-centric air quality data model
 * 
 * Data Layout:
 * ```
 * Station 1: [Record1, Record2, Record3, ...] (all measurements for this station)
 * Station 2: [Record1, Record2, Record3, ...] (all measurements for this station)
 * Station 3: [Record1, Record2, Record3, ...]
 * ...
 * ```
 * 
 * This row-oriented layout groups all measurements for each station together,
 * providing excellent performance for station-specific queries but requiring
 * more work for temporal aggregations across all stations.
 */
class RowModel {
private:
    // Station-centric storage: each station has vector of its records
    std::vector<std::vector<Record>> _stationRecords;
    
    // Station metadata
    std::vector<StationInfo> _stations;
    
    // Fast lookups
    std::unordered_map<std::string, int> _siteIdToIndex;  ///< SiteID -> station index
    
    // Time range (for quick filtering)
    long long _minTimestamp;
    long long _maxTimestamp;
    
    // Pollutant types present in data
    std::vector<std::string> _pollutantTypes;

public:
    /**
     * @brief Default constructor
     */
    RowModel() : _minTimestamp(0), _maxTimestamp(0) {}
    
    /**
     * @brief Build model from loaded file data
     * @param fileResults Vector of loaded file results
     * 
     * Groups all records by station (using siteId1 as key).
     * Sorts each station's records by timestamp for efficient queries.
     * Builds lookup indices for O(1) station access.
     */
    void buildFromFiles(const std::vector<FileLoadResult>& fileResults);
    
    // === Metadata Queries ===
    
    /**
     * @brief Get total number of stations
     */
    size_t stationCount() const { return _stations.size(); }
    
    /**
     * @brief Get total number of records across all stations
     */
    size_t totalRecords() const;
    
    /**
     * @brief Get time range covered by data
     */
    std::pair<long long, long long> timeRange() const {
        return {_minTimestamp, _maxTimestamp};
    }
    
    /**
     * @brief Get all unique pollutant types in dataset
     */
    const std::vector<std::string>& pollutantTypes() const { 
        return _pollutantTypes; 
    }
    
    /**
     * @brief Get station metadata
     */
    const std::vector<StationInfo>& stations() const { 
        return _stations; 
    }
    
    /**
     * @brief Get station info by index
     */
    const StationInfo& getStation(int index) const {
        return _stations[index];
    }
    
    // === Data Access ===
    
    /**
     * @brief Get all records for a specific station by index
     * @param stationIndex Station index (0-based)
     * @return Const reference to vector of records for this station
     */
    const std::vector<Record>& getStationRecords(int stationIndex) const {
        return _stationRecords[stationIndex];
    }
    
    /**
     * @brief Get all records for a specific station by site ID
     * @param siteId Station site identifier
     * @return Const reference to vector of records, or empty if not found
     */
    const std::vector<Record>& getStationRecordsBySiteId(const std::string& siteId) const;
    
    /**
     * @brief Find station index by site ID
     * @param siteId Station site identifier
     * @return Station index, or -1 if not found
     */
    int findStationIndex(const std::string& siteId) const;
    
    // === Raw Data Access (for service layer) ===
    
    /**
     * @brief Get direct access to all station records
     * 
     * Used by service layer for efficient parallel iteration.
     * Each element is one station's complete record vector.
     */
    const std::vector<std::vector<Record>>& allStationRecords() const {
        return _stationRecords;
    }
    
    /**
     * @brief Print model statistics
     */
    void printStats() const;

private:
    /**
     * @brief Extract unique pollutant types from all records
     */
    void extractPollutantTypes();
};

} // namespace AirQuality


#pragma once

#include "airquality_types.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <map>

/**
 * @file airquality_model_column.hpp
 * @brief Column-oriented (time-centric) air quality data model
 * 
 * This model organizes data by timestamp, where each time slot contains all
 * station measurements for that moment. This layout is optimal for:
 * - Temporal aggregations across all stations
 * - Time-slice queries (all stations at specific time)
 * - Parallel reductions over contiguous memory
 * 
 * Trade-offs:
 * + Excellent for cross-station temporal aggregations
 * + Superior cache locality for time-slice operations
 * + Better vectorization and parallel reduction performance
 * - Less efficient for station-specific time-series queries
 * - Requires scanning multiple time slots for per-station analysis
 */

namespace AirQuality {

/**
 * @class ColumnModel
 * @brief Time-centric air quality data model
 * 
 * Data Layout:
 * ```
 * Timestamp 1: [Station1_Record, Station2_Record, Station3_Record, ...]
 * Timestamp 2: [Station1_Record, Station2_Record, Station3_Record, ...]
 * Timestamp 3: [Station1_Record, Station2_Record, Station3_Record, ...]
 * ...
 * ```
 * 
 * This column-oriented layout groups all measurements for each timestamp together,
 * providing excellent performance for temporal queries but requiring more work
 * for station-specific analyses.
 */
class ColumnModel {
private:
    // Time-centric storage: each timestamp has vector of all station records
    std::vector<std::vector<Record>> _timeSlots;
    
    // Sorted timestamps (one per time slot)
    std::vector<long long> _timestamps;
    
    // Fast lookups
    std::unordered_map<long long, int> _timestampToIndex;  ///< Timestamp -> time slot index
    
    // Station metadata (for reference)
    std::vector<StationInfo> _stations;
    std::unordered_map<std::string, int> _siteIdToIndex;  ///< SiteID -> station index
    
    // Pollutant types present in data
    std::vector<std::string> _pollutantTypes;

public:
    /**
     * @brief Default constructor
     */
    ColumnModel() {}
    
    /**
     * @brief Build model from loaded file data
     * @param fileResults Vector of loaded file results
     * 
     * Groups all records by timestamp.
     * Maintains station metadata for cross-referencing.
     * Builds lookup indices for O(1) time slot access.
     */
    void buildFromFiles(const std::vector<FileLoadResult>& fileResults);
    
    // === Metadata Queries ===
    
    /**
     * @brief Get total number of time slots
     */
    size_t timeSlotCount() const { return _timestamps.size(); }
    
    /**
     * @brief Get total number of unique stations
     */
    size_t stationCount() const { return _stations.size(); }
    
    /**
     * @brief Get total number of records across all time slots
     */
    size_t totalRecords() const;
    
    /**
     * @brief Get all timestamps (sorted)
     */
    const std::vector<long long>& timestamps() const { 
        return _timestamps; 
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
    
    // === Data Access ===
    
    /**
     * @brief Get all records for a specific time slot by index
     * @param timeIndex Time slot index (0-based)
     * @return Const reference to vector of records at this time
     */
    const std::vector<Record>& getRecordsAtTime(int timeIndex) const {
        return _timeSlots[timeIndex];
    }
    
    /**
     * @brief Get all records for a specific timestamp
     * @param timestamp Unix timestamp
     * @return Const reference to vector of records, or empty if not found
     */
    const std::vector<Record>& getRecordsAtTimestamp(long long timestamp) const;
    
    /**
     * @brief Find time slot index for timestamp
     * @param timestamp Unix timestamp
     * @return Time slot index, or -1 if not found
     */
    int findTimeIndex(long long timestamp) const;
    
    /**
     * @brief Find time range indices (for range queries)
     * @param startTime Start timestamp (inclusive)
     * @param endTime End timestamp (inclusive)
     * @return Pair of (startIndex, endIndex), or (-1, -1) if no overlap
     */
    std::pair<int, int> findTimeRange(long long startTime, long long endTime) const;
    
    // === Raw Data Access (for service layer) ===
    
    /**
     * @brief Get direct access to all time slots
     * 
     * Used by service layer for efficient parallel iteration.
     * Each element is one timestamp's complete record vector.
     */
    const std::vector<std::vector<Record>>& allTimeSlots() const {
        return _timeSlots;
    }
    
    /**
     * @brief Print model statistics
     */
    void printStats() const;

private:
    /**
     * @brief Build station metadata from records
     */
    void buildStationMetadata(const std::vector<Record>& allRecords);
    
    /**
     * @brief Extract unique pollutant types from all records
     */
    void extractPollutantTypes();
};

} // namespace AirQuality


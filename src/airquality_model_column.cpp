#include "../interface/airquality_model_column.hpp"
#include <iostream>
#include <algorithm>
#include <unordered_set>

namespace AirQuality {

void ColumnModel::buildFromFiles(const std::vector<FileLoadResult>& fileResults) {
    std::cout << "Building column-oriented model (time-centric)...\n";
    
    // Step 1: Collect all records from all files
    std::vector<Record> allRecords;
    size_t totalExpected = 0;
    
    for (const auto& fileResult : fileResults) {
        if (fileResult.success) {
            totalExpected += fileResult.recordCount;
        }
    }
    
    allRecords.reserve(totalExpected);
    
    for (const auto& fileResult : fileResults) {
        if (fileResult.success) {
            allRecords.insert(allRecords.end(),
                            fileResult.records.begin(),
                            fileResult.records.end());
        }
    }
    
    std::cout << "  Collected " << allRecords.size() << " records from " 
              << fileResults.size() << " files\n";
    
    if (allRecords.empty()) {
        std::cerr << "  WARNING: No records to process!\n";
        return;
    }
    
    // Step 2: Group records by timestamp (use map for automatic sorting)
    std::map<long long, std::vector<Record>> timeMap;
    
    for (auto& record : allRecords) {
        timeMap[record.timestamp].push_back(std::move(record));
    }
    
    std::cout << "  Grouped into " << timeMap.size() << " unique timestamps\n";
    
    // Step 3: Build time-centric storage
    _timestamps.clear();
    _timeSlots.clear();
    _timestampToIndex.clear();
    
    _timestamps.reserve(timeMap.size());
    _timeSlots.reserve(timeMap.size());
    
    int timeIndex = 0;
    for (auto& [timestamp, records] : timeMap) {
        _timestamps.push_back(timestamp);
        _timeSlots.push_back(std::move(records));
        _timestampToIndex[timestamp] = timeIndex++;
    }
    
    // Step 4: Build station metadata (aggregate from all records)
    buildStationMetadata(allRecords);
    
    // Step 5: Extract pollutant types
    extractPollutantTypes();
    
    std::cout << "  ✅ Column model built successfully!\n";
    printStats();
}

size_t ColumnModel::totalRecords() const {
    size_t total = 0;
    for (const auto& timeSlot : _timeSlots) {
        total += timeSlot.size();
    }
    return total;
}

const std::vector<Record>& ColumnModel::getRecordsAtTimestamp(long long timestamp) const {
    static const std::vector<Record> empty;
    
    auto it = _timestampToIndex.find(timestamp);
    if (it == _timestampToIndex.end()) {
        return empty;
    }
    
    return _timeSlots[it->second];
}

int ColumnModel::findTimeIndex(long long timestamp) const {
    auto it = _timestampToIndex.find(timestamp);
    return (it != _timestampToIndex.end()) ? it->second : -1;
}

std::pair<int, int> ColumnModel::findTimeRange(long long startTime, long long endTime) const {
    if (_timestamps.empty()) {
        return {-1, -1};
    }
    
    // Binary search for start and end indices
    auto startIt = std::lower_bound(_timestamps.begin(), _timestamps.end(), startTime);
    auto endIt = std::upper_bound(_timestamps.begin(), _timestamps.end(), endTime);
    
    if (startIt == _timestamps.end()) {
        return {-1, -1};  // Start time is beyond all data
    }
    
    int startIndex = static_cast<int>(startIt - _timestamps.begin());
    int endIndex = static_cast<int>(endIt - _timestamps.begin()) - 1;
    
    if (endIndex < startIndex) {
        return {-1, -1};  // No overlap
    }
    
    return {startIndex, endIndex};
}

void ColumnModel::buildStationMetadata(const std::vector<Record>& allRecords) {
    // Group by station to count records and get metadata
    std::unordered_map<std::string, StationInfo> stationMap;
    
    for (const auto& record : allRecords) {
        auto& info = stationMap[record.siteId1];
        
        if (info.siteId.empty()) {
            // First time seeing this station
            info.siteId = record.siteId1;
            info.location = record.location;
            info.latitude = record.latitude;
            info.longitude = record.longitude;
            info.agency = record.agency;
            info.recordCount = 0;
        }
        
        info.recordCount++;
    }
    
    // Convert to vectors
    _stations.clear();
    _siteIdToIndex.clear();
    _stations.reserve(stationMap.size());
    
    int stationIndex = 0;
    for (auto& [siteId, info] : stationMap) {
        _stations.push_back(info);
        _siteIdToIndex[siteId] = stationIndex++;
    }
    
    // Sort stations by siteId for consistent ordering
    std::sort(_stations.begin(), _stations.end(),
             [](const StationInfo& a, const StationInfo& b) {
                 return a.siteId < b.siteId;
             });
    
    // Rebuild index after sorting
    _siteIdToIndex.clear();
    for (size_t i = 0; i < _stations.size(); i++) {
        _siteIdToIndex[_stations[i].siteId] = static_cast<int>(i);
    }
}

void ColumnModel::extractPollutantTypes() {
    std::unordered_set<std::string> uniquePollutants;
    
    // Sample from first few time slots to find pollutant types
    size_t samplesToCheck = std::min(size_t(10), _timeSlots.size());
    
    for (size_t i = 0; i < samplesToCheck; i++) {
        for (const auto& record : _timeSlots[i]) {
            uniquePollutants.insert(record.pollutant);
        }
    }
    
    _pollutantTypes.assign(uniquePollutants.begin(), uniquePollutants.end());
    std::sort(_pollutantTypes.begin(), _pollutantTypes.end());
}

void ColumnModel::printStats() const {
    std::cout << "\n  📊 Column Model Statistics:\n";
    std::cout << "     Time Slots: " << timeSlotCount() << "\n";
    std::cout << "     Stations: " << stationCount() << "\n";
    std::cout << "     Total Records: " << totalRecords() << "\n";
    
    if (!_timestamps.empty()) {
        std::cout << "     Time Range: " << _timestamps.front() 
                  << " to " << _timestamps.back() << "\n";
    }
    
    std::cout << "     Pollutant Types: ";
    for (size_t i = 0; i < _pollutantTypes.size(); i++) {
        std::cout << _pollutantTypes[i];
        if (i < _pollutantTypes.size() - 1) std::cout << ", ";
    }
    std::cout << "\n";
    
    // Show record distribution per time slot
    if (!_timeSlots.empty()) {
        size_t minRecords = SIZE_MAX;
        size_t maxRecords = 0;
        size_t totalRecs = 0;
        
        for (const auto& timeSlot : _timeSlots) {
            size_t count = timeSlot.size();
            minRecords = std::min(minRecords, count);
            maxRecords = std::max(maxRecords, count);
            totalRecs += count;
        }
        
        double avgRecords = static_cast<double>(totalRecs) / _timeSlots.size();
        
        std::cout << "     Records per Time Slot: min=" << minRecords 
                  << ", max=" << maxRecords 
                  << ", avg=" << static_cast<size_t>(avgRecords) << "\n";
    }
    std::cout << "\n";
}

} // namespace AirQuality


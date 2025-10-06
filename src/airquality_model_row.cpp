#include "../interface/airquality_model_row.hpp"
#include <iostream>
#include <algorithm>
#include <unordered_set>

namespace AirQuality {

void RowModel::buildFromFiles(const std::vector<FileLoadResult>& fileResults) {
    std::cout << "Building row-oriented model (station-centric)...\n";
    
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
    
    // Step 2: Group records by station (using siteId1 as key)
    std::unordered_map<std::string, std::vector<Record>> stationMap;
    
    for (auto& record : allRecords) {
        stationMap[record.siteId1].push_back(std::move(record));
    }
    
    std::cout << "  Grouped into " << stationMap.size() << " unique stations\n";
    
    // Step 3: Build station-centric storage
    _stations.clear();
    _stationRecords.clear();
    _siteIdToIndex.clear();
    
    _stations.reserve(stationMap.size());
    _stationRecords.reserve(stationMap.size());
    
    int stationIndex = 0;
    _minTimestamp = LLONG_MAX;
    _maxTimestamp = LLONG_MIN;
    
    for (auto& [siteId, records] : stationMap) {
        // Sort this station's records by timestamp
        std::sort(records.begin(), records.end(),
                 [](const Record& a, const Record& b) {
                     return a.timestamp < b.timestamp;
                 });
        
        // Create station info
        StationInfo info;
        info.siteId = siteId;
        info.location = records[0].location;
        info.latitude = records[0].latitude;
        info.longitude = records[0].longitude;
        info.agency = records[0].agency;
        info.recordCount = records.size();
        
        // Update time range
        if (!records.empty()) {
            _minTimestamp = std::min(_minTimestamp, records.front().timestamp);
            _maxTimestamp = std::max(_maxTimestamp, records.back().timestamp);
        }
        
        // Store
        _stations.push_back(info);
        _stationRecords.push_back(std::move(records));
        _siteIdToIndex[siteId] = stationIndex++;
    }
    
    // Step 4: Extract pollutant types
    extractPollutantTypes();
    
    std::cout << "  ✅ Row model built successfully!\n";
    printStats();
}

size_t RowModel::totalRecords() const {
    size_t total = 0;
    for (const auto& stationRecords : _stationRecords) {
        total += stationRecords.size();
    }
    return total;
}

const std::vector<Record>& RowModel::getStationRecordsBySiteId(const std::string& siteId) const {
    static const std::vector<Record> empty;
    
    auto it = _siteIdToIndex.find(siteId);
    if (it == _siteIdToIndex.end()) {
        return empty;
    }
    
    return _stationRecords[it->second];
}

int RowModel::findStationIndex(const std::string& siteId) const {
    auto it = _siteIdToIndex.find(siteId);
    return (it != _siteIdToIndex.end()) ? it->second : -1;
}

void RowModel::extractPollutantTypes() {
    std::unordered_set<std::string> uniquePollutants;
    
    // Sample from first few stations to find pollutant types
    size_t samplesToCheck = std::min(size_t(10), _stationRecords.size());
    
    for (size_t i = 0; i < samplesToCheck; i++) {
        for (const auto& record : _stationRecords[i]) {
            uniquePollutants.insert(record.pollutant);
        }
    }
    
    _pollutantTypes.assign(uniquePollutants.begin(), uniquePollutants.end());
    std::sort(_pollutantTypes.begin(), _pollutantTypes.end());
}

void RowModel::printStats() const {
    std::cout << "\n  📊 Row Model Statistics:\n";
    std::cout << "     Stations: " << stationCount() << "\n";
    std::cout << "     Total Records: " << totalRecords() << "\n";
    std::cout << "     Time Range: " << _minTimestamp << " to " << _maxTimestamp << "\n";
    std::cout << "     Pollutant Types: ";
    for (size_t i = 0; i < _pollutantTypes.size(); i++) {
        std::cout << _pollutantTypes[i];
        if (i < _pollutantTypes.size() - 1) std::cout << ", ";
    }
    std::cout << "\n";
    
    // Show record distribution
    if (!_stationRecords.empty()) {
        size_t minRecords = SIZE_MAX;
        size_t maxRecords = 0;
        size_t totalRecs = 0;
        
        for (const auto& stationRecs : _stationRecords) {
            size_t count = stationRecs.size();
            minRecords = std::min(minRecords, count);
            maxRecords = std::max(maxRecords, count);
            totalRecs += count;
        }
        
        double avgRecords = static_cast<double>(totalRecs) / _stationRecords.size();
        
        std::cout << "     Records per Station: min=" << minRecords 
                  << ", max=" << maxRecords 
                  << ", avg=" << static_cast<size_t>(avgRecords) << "\n";
    }
    std::cout << "\n";
}

} // namespace AirQuality


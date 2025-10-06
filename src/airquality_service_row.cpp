#include "../interface/airquality_service_row.hpp"
#include <omp.h>
#include <algorithm>
#include <cmath>

namespace AirQuality {

double RowService::avgPollutantAtTime(long long timestamp, const std::string& pollutant, int numThreads) const {
    // Row model disadvantage: Must scan ALL stations for specific timestamp
    const auto& stationRecords = _model->allStationRecords();
    
    double sum = 0.0;
    int count = 0;
    
    // Parallel reduction across stations
    #pragma omp parallel for reduction(+:sum,count) num_threads(numThreads)
    for (size_t s = 0; s < stationRecords.size(); s++) {
        const auto& records = stationRecords[s];
        
        // Binary search or linear scan for timestamp
        for (const auto& record : records) {
            if (record.timestamp == timestamp && record.pollutant == pollutant && 
                std::isfinite(record.value)) {
                sum += record.value;
                count++;
                break;  // Found match for this station at this time
            }
        }
    }
    
    return (count > 0) ? (sum / count) : 0.0;
}

double RowService::maxPollutantAtTime(long long timestamp, const std::string& pollutant, int numThreads) const {
    const auto& stationRecords = _model->allStationRecords();
    
    double maxVal = -INFINITY;
    
    #pragma omp parallel for reduction(max:maxVal) num_threads(numThreads)
    for (size_t s = 0; s < stationRecords.size(); s++) {
        const auto& records = stationRecords[s];
        
        for (const auto& record : records) {
            if (record.timestamp == timestamp && record.pollutant == pollutant && 
                std::isfinite(record.value)) {
                if (record.value > maxVal) {
                    maxVal = record.value;
                }
                break;
            }
        }
    }
    
    return (maxVal == -INFINITY) ? 0.0 : maxVal;
}

double RowService::minPollutantAtTime(long long timestamp, const std::string& pollutant, int numThreads) const {
    const auto& stationRecords = _model->allStationRecords();
    
    double minVal = INFINITY;
    
    #pragma omp parallel for reduction(min:minVal) num_threads(numThreads)
    for (size_t s = 0; s < stationRecords.size(); s++) {
        const auto& records = stationRecords[s];
        
        for (const auto& record : records) {
            if (record.timestamp == timestamp && record.pollutant == pollutant && 
                std::isfinite(record.value)) {
                if (record.value < minVal) {
                    minVal = record.value;
                }
                break;
            }
        }
    }
    
    return (minVal == INFINITY) ? 0.0 : minVal;
}

std::vector<std::pair<long long, double>> RowService::timeSeriesForStation(
    const std::string& siteId, const std::string& pollutant, int numThreads) const {
    
    // Row model advantage: Direct station access!
    const auto& records = _model->getStationRecordsBySiteId(siteId);
    
    std::vector<std::pair<long long, double>> result;
    result.reserve(records.size());
    
    // Filter by pollutant (records already sorted by time)
    for (const auto& record : records) {
        if (record.pollutant == pollutant) {
            result.emplace_back(record.timestamp, record.value);
        }
    }
    
    return result;
}

double RowService::avgForStationInRange(
    const std::string& siteId, long long startTime, long long endTime,
    const std::string& pollutant, int numThreads) const {
    
    // Row model advantage: Direct station access!
    const auto& records = _model->getStationRecordsBySiteId(siteId);
    
    double sum = 0.0;
    int count = 0;
    
    // Records are sorted by timestamp, so we can stop early
    for (const auto& record : records) {
        if (record.timestamp > endTime) break;  // Past range
        
        if (record.timestamp >= startTime && record.timestamp <= endTime &&
            record.pollutant == pollutant && std::isfinite(record.value)) {
            sum += record.value;
            count++;
        }
    }
    
    return (count > 0) ? (sum / count) : 0.0;
}

std::vector<std::pair<std::string, double>> RowService::topNStationsAtTime(
    int n, long long timestamp, const std::string& pollutant, int numThreads) const {
    
    const auto& stationRecords = _model->allStationRecords();
    const auto& stations = _model->stations();
    
    if (n <= 0) return {};
    
    // Collect matching records from all stations
    std::vector<std::pair<std::string, double>> candidates;
    candidates.reserve(stationRecords.size());
    
    for (size_t s = 0; s < stationRecords.size(); s++) {
        const auto& records = stationRecords[s];
        
        for (const auto& record : records) {
            if (record.timestamp == timestamp && record.pollutant == pollutant && 
                std::isfinite(record.value)) {
                candidates.emplace_back(stations[s].siteId, record.value);
                break;
            }
        }
    }
    
    // Sort by value descending
    std::sort(candidates.begin(), candidates.end(),
             [](const auto& a, const auto& b) {
                 return a.second > b.second;
             });
    
    // Return top N
    size_t resultSize = std::min(static_cast<size_t>(n), candidates.size());
    return std::vector<std::pair<std::string, double>>(
        candidates.begin(), candidates.begin() + resultSize);
}

size_t RowService::countRecords(long long startTime, long long endTime, const std::string& pollutant) const {
    const auto& stationRecords = _model->allStationRecords();
    
    size_t count = 0;
    
    for (const auto& records : stationRecords) {
        for (const auto& record : records) {
            if (record.timestamp > endTime) break;  // Past range for this station
            
            if (record.timestamp >= startTime && record.timestamp <= endTime &&
                record.pollutant == pollutant) {
                count++;
            }
        }
    }
    
    return count;
}

} // namespace AirQuality


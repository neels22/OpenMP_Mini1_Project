#include "../interface/airquality_service_column.hpp"
#include <omp.h>
#include <algorithm>
#include <queue>
#include <cmath>

namespace AirQuality {

double ColumnService::avgPollutantAtTime(long long timestamp, const std::string& pollutant, int numThreads) const {
    // Column model advantage: Direct access to time slot!
    const auto& records = _model->getRecordsAtTimestamp(timestamp);
    
    if (records.empty()) return 0.0;
    
    double sum = 0.0;
    int count = 0;
    
    // OpenMP parallel reduction - THIS IS WHERE SPEEDUP HAPPENS!
    #pragma omp parallel for reduction(+:sum,count) num_threads(numThreads)
    for (size_t i = 0; i < records.size(); i++) {
        if (records[i].pollutant == pollutant && std::isfinite(records[i].value)) {
            sum += records[i].value;
            count++;
        }
    }
    
    return (count > 0) ? (sum / count) : 0.0;
}

double ColumnService::maxPollutantAtTime(long long timestamp, const std::string& pollutant, int numThreads) const {
    const auto& records = _model->getRecordsAtTimestamp(timestamp);
    
    if (records.empty()) return 0.0;
    
    double maxVal = -INFINITY;
    
    #pragma omp parallel for reduction(max:maxVal) num_threads(numThreads)
    for (size_t i = 0; i < records.size(); i++) {
        if (records[i].pollutant == pollutant && std::isfinite(records[i].value)) {
            if (records[i].value > maxVal) {
                maxVal = records[i].value;
            }
        }
    }
    
    return (maxVal == -INFINITY) ? 0.0 : maxVal;
}

double ColumnService::minPollutantAtTime(long long timestamp, const std::string& pollutant, int numThreads) const {
    const auto& records = _model->getRecordsAtTimestamp(timestamp);
    
    if (records.empty()) return 0.0;
    
    double minVal = INFINITY;
    
    #pragma omp parallel for reduction(min:minVal) num_threads(numThreads)
    for (size_t i = 0; i < records.size(); i++) {
        if (records[i].pollutant == pollutant && std::isfinite(records[i].value)) {
            if (records[i].value < minVal) {
                minVal = records[i].value;
            }
        }
    }
    
    return (minVal == INFINITY) ? 0.0 : minVal;
}

std::vector<std::pair<long long, double>> ColumnService::timeSeriesForStation(
    const std::string& siteId, const std::string& pollutant, int numThreads) const {
    
    // Column model disadvantage: Must scan ALL time slots
    const auto& timeSlots = _model->allTimeSlots();
    const auto& timestamps = _model->timestamps();
    
    std::vector<std::pair<long long, double>> result;
    result.reserve(timestamps.size());
    
    // Can't easily parallelize this - need to preserve order
    // (Could use parallel sections with merging, but overhead likely not worth it)
    for (size_t t = 0; t < timeSlots.size(); t++) {
        const auto& records = timeSlots[t];
        
        for (const auto& record : records) {
            if (record.siteId1 == siteId && record.pollutant == pollutant) {
                result.emplace_back(timestamps[t], record.value);
                break;  // Found match for this timestamp
            }
        }
    }
    
    return result;
}

double ColumnService::avgForStationInRange(
    const std::string& siteId, long long startTime, long long endTime,
    const std::string& pollutant, int numThreads) const {
    
    std::pair<int, int> range = _model->findTimeRange(startTime, endTime);
    int startIdx = range.first;
    int endIdx = range.second;
    
    if (startIdx < 0) return 0.0;
    
    const auto& timeSlots = _model->allTimeSlots();
    
    double sum = 0.0;
    int count = 0;
    
    // Parallel reduction over time range
    #pragma omp parallel for reduction(+:sum,count) num_threads(numThreads)
    for (int t = startIdx; t <= endIdx; t++) {
        const auto& records = timeSlots[t];
        
        for (const auto& record : records) {
            if (record.siteId1 == siteId && record.pollutant == pollutant && 
                std::isfinite(record.value)) {
                sum += record.value;
                count++;
            }
        }
    }
    
    return (count > 0) ? (sum / count) : 0.0;
}

std::vector<std::pair<std::string, double>> ColumnService::topNStationsAtTime(
    int n, long long timestamp, const std::string& pollutant, int numThreads) const {
    
    const auto& records = _model->getRecordsAtTimestamp(timestamp);
    
    if (records.empty() || n <= 0) {
        return {};
    }
    
    // Collect matching records
    std::vector<std::pair<std::string, double>> candidates;
    candidates.reserve(records.size());
    
    for (const auto& record : records) {
        if (record.pollutant == pollutant && std::isfinite(record.value)) {
            candidates.emplace_back(record.siteId1, record.value);
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

size_t ColumnService::countRecords(long long startTime, long long endTime, const std::string& pollutant) const {
    std::pair<int, int> range = _model->findTimeRange(startTime, endTime);
    int startIdx = range.first;
    int endIdx = range.second;
    
    if (startIdx < 0) return 0;
    
    const auto& timeSlots = _model->allTimeSlots();
    size_t count = 0;
    
    for (int t = startIdx; t <= endIdx; t++) {
        const auto& records = timeSlots[t];
        
        for (const auto& record : records) {
            if (record.pollutant == pollutant) {
                count++;
            }
        }
    }
    
    return count;
}

} // namespace AirQuality


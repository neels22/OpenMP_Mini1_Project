#pragma once

#include "airquality_service_interface.hpp"
#include "airquality_model_row.hpp"

namespace AirQuality {

class RowService : public IAirQualityService {
private:
    const RowModel* _model;

public:
    explicit RowService(const RowModel* model) : _model(model) {}
    
    double avgPollutantAtTime(long long timestamp, const std::string& pollutant, int numThreads = 1) const override;
    double maxPollutantAtTime(long long timestamp, const std::string& pollutant, int numThreads = 1) const override;
    double minPollutantAtTime(long long timestamp, const std::string& pollutant, int numThreads = 1) const override;
    
    std::vector<std::pair<long long, double>> timeSeriesForStation(
        const std::string& siteId, const std::string& pollutant, int numThreads = 1) const override;
    
    double avgForStationInRange(const std::string& siteId, long long startTime, long long endTime,
                               const std::string& pollutant, int numThreads = 1) const override;
    
    std::vector<std::pair<std::string, double>> topNStationsAtTime(
        int n, long long timestamp, const std::string& pollutant, int numThreads = 1) const override;
    
    size_t countRecords(long long startTime, long long endTime, const std::string& pollutant) const override;
    
    std::string getImplementationName() const override {
        return "Row-oriented (Station-centric)";
    }
};

} // namespace AirQuality

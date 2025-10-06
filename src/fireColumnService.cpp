#include "../interface/fire_service_direct.hpp"
#include "../interface/fireColumnModel.hpp"
#include <algorithm>
#include <numeric>
#include <queue>
#include <functional>
#include <omp.h>
#include <limits>
#include <unordered_map>

FireColumnService::FireColumnService(const FireColumnModel* model) : model_(model) {}
FireColumnService::~FireColumnService() = default;

std::string FireColumnService::getImplementationName() const {
    return "Fire Column-oriented";
}

std::size_t FireColumnService::totalMeasurementCount() const {
    return model_->measurementCount();
}

std::size_t FireColumnService::uniqueSiteCount() const {
    return model_->siteCount();
}

int FireColumnService::maxAQI(int numThreads) const {
    const auto& aqis = model_->aqis();
    if (aqis.empty()) return 0;
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        int global_max = std::numeric_limits<int>::min();
        
#pragma omp parallel for reduction(max:global_max)
        for (std::size_t i = 0; i < aqis.size(); ++i) {
            global_max = std::max(global_max, aqis[i]);
        }
        return global_max == std::numeric_limits<int>::min() ? 0 : global_max;
    }
    
    // Serial version
    int maxAQIValue = 0;
    for (std::size_t i = 0; i < aqis.size(); ++i) {
        maxAQIValue = std::max(maxAQIValue, aqis[i]);
    }
    return maxAQIValue;
}

int FireColumnService::minAQI(int numThreads) const {
    const auto& aqis = model_->aqis();
    if (aqis.empty()) return 0;
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        int global_min = std::numeric_limits<int>::max();
        
#pragma omp parallel for reduction(min:global_min)
        for (std::size_t i = 0; i < aqis.size(); ++i) {
            int aqi = aqis[i];
            if (aqi > 0) { // Only consider valid AQI values
                global_min = std::min(global_min, aqi);
            }
        }
        return global_min == std::numeric_limits<int>::max() ? 0 : global_min;
    }
    
    // Serial version
    int minAQIValue = std::numeric_limits<int>::max();
    for (std::size_t i = 0; i < aqis.size(); ++i) {
        int aqi = aqis[i];
        if (aqi > 0) { // Only consider valid AQI values
            minAQIValue = std::min(minAQIValue, aqi);
        }
    }
    return minAQIValue == std::numeric_limits<int>::max() ? 0 : minAQIValue;
}

double FireColumnService::averageAQI(int numThreads) const {
    const auto& aqis = model_->aqis();
    if (aqis.empty()) return 0.0;
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        long long total = 0;
        
#pragma omp parallel for reduction(+:total)
        for (std::size_t i = 0; i < aqis.size(); ++i) {
            total += aqis[i];
        }
        return static_cast<double>(total) / static_cast<double>(aqis.size());
    }
    
    // Serial version
    long long total = 0;
    for (std::size_t i = 0; i < aqis.size(); ++i) {
        total += aqis[i];
    }
    return static_cast<double>(total) / static_cast<double>(aqis.size());
}

std::vector<std::pair<std::string, double>> FireColumnService::topNSitesByAverageConcentration(std::size_t n, int numThreads) const {
    if (n == 0) return {};
    
    const auto& siteNames = model_->siteNames();
    const auto& concentrations = model_->concentrations();
    
    if (siteNames.empty() || concentrations.empty()) return {};
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        
        // First, collect all site concentrations in parallel
        std::unordered_map<std::string, std::pair<double, std::size_t>> siteData; // site_name -> (total_concentration, count)
        
        // Use a mutex for thread-safe map access
        omp_lock_t mapLock;
        omp_init_lock(&mapLock);
        
#pragma omp parallel for
        for (std::size_t i = 0; i < siteNames.size(); ++i) {
            const std::string& siteName = siteNames[i];
            double concentration = concentrations[i];
            
            omp_set_lock(&mapLock);
            auto& data = siteData[siteName];
            data.first += concentration;
            data.second += 1;
            omp_unset_lock(&mapLock);
        }
        
        omp_destroy_lock(&mapLock);
        
        // Calculate averages and sort
        std::vector<std::pair<std::string, double>> siteAvgConcentrations;
        siteAvgConcentrations.reserve(siteData.size());
        
        for (const auto& entry : siteData) {
            if (entry.second.second > 0) {
                double avgConcentration = entry.second.first / entry.second.second;
                siteAvgConcentrations.emplace_back(entry.first, avgConcentration);
            }
        }
        
        // Sort descending by average concentration and take top-N
        std::sort(siteAvgConcentrations.begin(), siteAvgConcentrations.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        if (siteAvgConcentrations.size() > n) {
            siteAvgConcentrations.resize(n);
        }
        
        return siteAvgConcentrations;
    }
    
    // Serial version: collect all site concentrations
    std::unordered_map<std::string, std::pair<double, std::size_t>> siteData; // site_name -> (total_concentration, count)
    
    for (std::size_t i = 0; i < siteNames.size(); ++i) {
        const std::string& siteName = siteNames[i];
        double concentration = concentrations[i];
        
        auto& data = siteData[siteName];
        data.first += concentration;
        data.second += 1;
    }
    
    // Calculate averages and sort
    std::vector<std::pair<std::string, double>> siteAvgConcentrations;
    siteAvgConcentrations.reserve(siteData.size());
    
    for (const auto& entry : siteData) {
        if (entry.second.second > 0) {
            double avgConcentration = entry.second.first / entry.second.second;
            siteAvgConcentrations.emplace_back(entry.first, avgConcentration);
        }
    }
    
    // Sort descending by average concentration and take top-N
    std::sort(siteAvgConcentrations.begin(), siteAvgConcentrations.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    if (siteAvgConcentrations.size() > n) {
        siteAvgConcentrations.resize(n);
    }
    
    return siteAvgConcentrations;
}
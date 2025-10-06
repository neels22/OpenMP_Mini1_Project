#include "../interface/fire_service.hpp"
#include <algorithm>
#include <numeric>
#include <limits>
#include <cmath>
#include <queue>
#include <unordered_map>
#include <omp.h>

// ============================================================================
// FireRowModelService Implementation
// ============================================================================

FireRowModelService::FireRowModelService(const FireRowModel* model) : model_(model) {}
FireRowModelService::~FireRowModelService() = default;

std::string FireRowModelService::getImplementationName() const {
    return "Fire Row-oriented";
}

std::size_t FireRowModelService::totalMeasurementCount() const {
    return model_->totalMeasurements();
}

std::size_t FireRowModelService::uniqueSiteCount() const {
    return model_->siteCount();
}

// === Parameter-based Aggregations ===

double FireRowModelService::averageConcentrationForParameter(const std::string& parameter, int numThreads) const {
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        double total = 0.0;
        std::size_t count = 0;
        
        #pragma omp parallel for reduction(+:total,count)
        for (std::size_t i = 0; i < model_->siteCount(); ++i) {
            const auto& site = model_->siteAt(i);
            for (std::size_t j = 0; j < site.measurementCount(); ++j) {
                const auto& meas = site.getMeasurement(j);
                if (meas.parameter() == parameter) {
                    total += meas.concentration();
                    ++count;
                }
            }
        }
        return count > 0 ? total / count : 0.0;
    }
    
    // Serial implementation
    double total = 0.0;
    std::size_t count = 0;
    for (std::size_t i = 0; i < model_->siteCount(); ++i) {
        const auto& site = model_->siteAt(i);
        for (std::size_t j = 0; j < site.measurementCount(); ++j) {
            const auto& meas = site.getMeasurement(j);
            if (meas.parameter() == parameter) {
                total += meas.concentration();
                ++count;
            }
        }
    }
    return count > 0 ? total / count : 0.0;
}

double FireRowModelService::sumConcentrationsForParameter(const std::string& parameter, int numThreads) const {
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        double total = 0.0;
        
        #pragma omp parallel for reduction(+:total)
        for (std::size_t i = 0; i < model_->siteCount(); ++i) {
            const auto& site = model_->siteAt(i);
            for (std::size_t j = 0; j < site.measurementCount(); ++j) {
                const auto& meas = site.getMeasurement(j);
                if (meas.parameter() == parameter) {
                    total += meas.concentration();
                }
            }
        }
        return total;
    }
    
    // Serial implementation
    double total = 0.0;
    for (std::size_t i = 0; i < model_->siteCount(); ++i) {
        const auto& site = model_->siteAt(i);
        for (std::size_t j = 0; j < site.measurementCount(); ++j) {
            const auto& meas = site.getMeasurement(j);
            if (meas.parameter() == parameter) {
                total += meas.concentration();
            }
        }
    }
    return total;
}

double FireRowModelService::maxConcentrationForParameter(const std::string& parameter, int numThreads) const {
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        double global_max = -std::numeric_limits<double>::infinity();
        
        #pragma omp parallel
        {
            double local_max = -std::numeric_limits<double>::infinity();
            #pragma omp for nowait
            for (std::size_t i = 0; i < model_->siteCount(); ++i) {
                const auto& site = model_->siteAt(i);
                for (std::size_t j = 0; j < site.measurementCount(); ++j) {
                    const auto& meas = site.getMeasurement(j);
                    if (meas.parameter() == parameter) {
                        local_max = std::max(local_max, meas.concentration());
                    }
                }
            }
            #pragma omp critical
            { global_max = std::max(global_max, local_max); }
        }
        return std::isinf(global_max) ? 0.0 : global_max;
    }
    
    // Serial implementation
    double max_val = -std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < model_->siteCount(); ++i) {
        const auto& site = model_->siteAt(i);
        for (std::size_t j = 0; j < site.measurementCount(); ++j) {
            const auto& meas = site.getMeasurement(j);
            if (meas.parameter() == parameter) {
                max_val = std::max(max_val, meas.concentration());
            }
        }
    }
    return std::isinf(max_val) ? 0.0 : max_val;
}

double FireRowModelService::minConcentrationForParameter(const std::string& parameter, int numThreads) const {
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        double global_min = std::numeric_limits<double>::infinity();
        
        #pragma omp parallel
        {
            double local_min = std::numeric_limits<double>::infinity();
            #pragma omp for nowait
            for (std::size_t i = 0; i < model_->siteCount(); ++i) {
                const auto& site = model_->siteAt(i);
                for (std::size_t j = 0; j < site.measurementCount(); ++j) {
                    const auto& meas = site.getMeasurement(j);
                    if (meas.parameter() == parameter) {
                        local_min = std::min(local_min, meas.concentration());
                    }
                }
            }
            #pragma omp critical
            { global_min = std::min(global_min, local_min); }
        }
        return std::isinf(global_min) ? 0.0 : global_min;
    }
    
    // Serial implementation
    double min_val = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < model_->siteCount(); ++i) {
        const auto& site = model_->siteAt(i);
        for (std::size_t j = 0; j < site.measurementCount(); ++j) {
            const auto& meas = site.getMeasurement(j);
            if (meas.parameter() == parameter) {
                min_val = std::min(min_val, meas.concentration());
            }
        }
    }
    return std::isinf(min_val) ? 0.0 : min_val;
}

// === AQI Operations ===

double FireRowModelService::averageAQI(int numThreads) const {
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        long long total = 0;
        std::size_t count = 0;
        
        #pragma omp parallel for reduction(+:total,count)
        for (std::size_t i = 0; i < model_->siteCount(); ++i) {
            const auto& site = model_->siteAt(i);
            for (std::size_t j = 0; j < site.measurementCount(); ++j) {
                total += site.getMeasurement(j).aqi();
                ++count;
            }
        }
        return count > 0 ? static_cast<double>(total) / count : 0.0;
    }
    
    // Serial implementation
    long long total = 0;
    std::size_t count = 0;
    for (std::size_t i = 0; i < model_->siteCount(); ++i) {
        const auto& site = model_->siteAt(i);
        for (std::size_t j = 0; j < site.measurementCount(); ++j) {
            total += site.getMeasurement(j).aqi();
            ++count;
        }
    }
    return count > 0 ? static_cast<double>(total) / count : 0.0;
}

int FireRowModelService::maxAQI(int numThreads) const {
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        int global_max = std::numeric_limits<int>::min();
        
        #pragma omp parallel
        {
            int local_max = std::numeric_limits<int>::min();
            #pragma omp for nowait
            for (std::size_t i = 0; i < model_->siteCount(); ++i) {
                const auto& site = model_->siteAt(i);
                for (std::size_t j = 0; j < site.measurementCount(); ++j) {
                    local_max = std::max(local_max, site.getMeasurement(j).aqi());
                }
            }
            #pragma omp critical
            { global_max = std::max(global_max, local_max); }
        }
        return global_max == std::numeric_limits<int>::min() ? 0 : global_max;
    }
    
    // Serial implementation
    int max_val = std::numeric_limits<int>::min();
    for (std::size_t i = 0; i < model_->siteCount(); ++i) {
        const auto& site = model_->siteAt(i);
        for (std::size_t j = 0; j < site.measurementCount(); ++j) {
            max_val = std::max(max_val, site.getMeasurement(j).aqi());
        }
    }
    return max_val == std::numeric_limits<int>::min() ? 0 : max_val;
}

int FireRowModelService::minAQI(int numThreads) const {
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        int global_min = std::numeric_limits<int>::max();
        
        #pragma omp parallel
        {
            int local_min = std::numeric_limits<int>::max();
            #pragma omp for nowait
            for (std::size_t i = 0; i < model_->siteCount(); ++i) {
                const auto& site = model_->siteAt(i);
                for (std::size_t j = 0; j < site.measurementCount(); ++j) {
                    local_min = std::min(local_min, site.getMeasurement(j).aqi());
                }
            }
            #pragma omp critical
            { global_min = std::min(global_min, local_min); }
        }
        return global_min == std::numeric_limits<int>::max() ? 0 : global_min;
    }
    
    // Serial implementation
    int min_val = std::numeric_limits<int>::max();
    for (std::size_t i = 0; i < model_->siteCount(); ++i) {
        const auto& site = model_->siteAt(i);
        for (std::size_t j = 0; j < site.measurementCount(); ++j) {
            min_val = std::min(min_val, site.getMeasurement(j).aqi());
        }
    }
    return min_val == std::numeric_limits<int>::max() ? 0 : min_val;
}

double FireRowModelService::averageAQIForParameter(const std::string& parameter, int numThreads) const {
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        long long total = 0;
        std::size_t count = 0;
        
        #pragma omp parallel for reduction(+:total,count)
        for (std::size_t i = 0; i < model_->siteCount(); ++i) {
            const auto& site = model_->siteAt(i);
            for (std::size_t j = 0; j < site.measurementCount(); ++j) {
                const auto& meas = site.getMeasurement(j);
                if (meas.parameter() == parameter) {
                    total += meas.aqi();
                    ++count;
                }
            }
        }
        return count > 0 ? static_cast<double>(total) / count : 0.0;
    }
    
    // Serial implementation
    long long total = 0;
    std::size_t count = 0;
    for (std::size_t i = 0; i < model_->siteCount(); ++i) {
        const auto& site = model_->siteAt(i);
        for (std::size_t j = 0; j < site.measurementCount(); ++j) {
            const auto& meas = site.getMeasurement(j);
            if (meas.parameter() == parameter) {
                total += meas.aqi();
                ++count;
            }
        }
    }
    return count > 0 ? static_cast<double>(total) / count : 0.0;
}

// === Site-Specific Operations ===

double FireRowModelService::averageConcentrationForSite(const std::string& siteName, int numThreads) const {
    const FireSiteData* site = model_->getBySiteName(siteName);
    if (!site) return 0.0;
    
    if (numThreads > 1 && site->measurementCount() > 100) {
        omp_set_num_threads(numThreads);
        double total = 0.0;
        
        #pragma omp parallel for reduction(+:total)
        for (std::size_t i = 0; i < site->measurementCount(); ++i) {
            total += site->getMeasurement(i).concentration();
        }
        return site->measurementCount() > 0 ? total / site->measurementCount() : 0.0;
    }
    
    // Serial implementation
    double total = 0.0;
    for (std::size_t i = 0; i < site->measurementCount(); ++i) {
        total += site->getMeasurement(i).concentration();
    }
    return site->measurementCount() > 0 ? total / site->measurementCount() : 0.0;
}

std::size_t FireRowModelService::measurementCountForSite(const std::string& siteName) const {
    const FireSiteData* site = model_->getBySiteName(siteName);
    return site ? site->measurementCount() : 0;
}

// === Geographic Operations ===

std::size_t FireRowModelService::countMeasurementsInBounds(double minLat, double maxLat, 
                                                           double minLon, double maxLon,
                                                           int numThreads) const {
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        std::size_t count = 0;
        
        #pragma omp parallel for reduction(+:count)
        for (std::size_t i = 0; i < model_->siteCount(); ++i) {
            const auto& site = model_->siteAt(i);
            for (std::size_t j = 0; j < site.measurementCount(); ++j) {
                const auto& meas = site.getMeasurement(j);
                double lat = meas.latitude();
                double lon = meas.longitude();
                if (lat >= minLat && lat <= maxLat && lon >= minLon && lon <= maxLon) {
                    ++count;
                }
            }
        }
        return count;
    }
    
    // Serial implementation
    std::size_t count = 0;
    for (std::size_t i = 0; i < model_->siteCount(); ++i) {
        const auto& site = model_->siteAt(i);
        for (std::size_t j = 0; j < site.measurementCount(); ++j) {
            const auto& meas = site.getMeasurement(j);
            double lat = meas.latitude();
            double lon = meas.longitude();
            if (lat >= minLat && lat <= maxLat && lon >= minLon && lon <= maxLon) {
                ++count;
            }
        }
    }
    return count;
}

double FireRowModelService::averageConcentrationInBounds(double minLat, double maxLat, 
                                                         double minLon, double maxLon,
                                                         int numThreads) const {
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        double total = 0.0;
        std::size_t count = 0;
        
        #pragma omp parallel for reduction(+:total,count)
        for (std::size_t i = 0; i < model_->siteCount(); ++i) {
            const auto& site = model_->siteAt(i);
            for (std::size_t j = 0; j < site.measurementCount(); ++j) {
                const auto& meas = site.getMeasurement(j);
                double lat = meas.latitude();
                double lon = meas.longitude();
                if (lat >= minLat && lat <= maxLat && lon >= minLon && lon <= maxLon) {
                    total += meas.concentration();
                    ++count;
                }
            }
        }
        return count > 0 ? total / count : 0.0;
    }
    
    // Serial implementation
    double total = 0.0;
    std::size_t count = 0;
    for (std::size_t i = 0; i < model_->siteCount(); ++i) {
        const auto& site = model_->siteAt(i);
        for (std::size_t j = 0; j < site.measurementCount(); ++j) {
            const auto& meas = site.getMeasurement(j);
            double lat = meas.latitude();
            double lon = meas.longitude();
            if (lat >= minLat && lat <= maxLat && lon >= minLon && lon <= maxLon) {
                total += meas.concentration();
                ++count;
            }
        }
    }
    return count > 0 ? total / count : 0.0;
}

// === Top-N Operations ===

std::vector<std::pair<std::string, double>> FireRowModelService::topNSitesByAverageConcentration(std::size_t n, int numThreads) const {
    // Compute average concentration per site
    std::vector<std::pair<std::string, double>> site_averages;
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        std::vector<std::vector<std::pair<std::string, double>>> thread_results(omp_get_max_threads());
        
        #pragma omp parallel
        {
            int tid = omp_get_thread_num();
            #pragma omp for nowait
            for (std::size_t i = 0; i < model_->siteCount(); ++i) {
                const auto& site = model_->siteAt(i);
                if (site.measurementCount() > 0) {
                    double total = 0.0;
                    for (std::size_t j = 0; j < site.measurementCount(); ++j) {
                        total += site.getMeasurement(j).concentration();
                    }
                    double avg = total / site.measurementCount();
                    thread_results[tid].emplace_back(site.siteIdentifier(), avg);
                }
            }
        }
        
        // Merge thread results
        for (const auto& tr : thread_results) {
            site_averages.insert(site_averages.end(), tr.begin(), tr.end());
        }
    } else {
        // Serial implementation
        for (std::size_t i = 0; i < model_->siteCount(); ++i) {
            const auto& site = model_->siteAt(i);
            if (site.measurementCount() > 0) {
                double total = 0.0;
                for (std::size_t j = 0; j < site.measurementCount(); ++j) {
                    total += site.getMeasurement(j).concentration();
                }
                double avg = total / site.measurementCount();
                site_averages.emplace_back(site.siteIdentifier(), avg);
            }
        }
    }
    
    // Sort and return top N
    std::partial_sort(site_averages.begin(), 
                     site_averages.begin() + std::min(n, site_averages.size()),
                     site_averages.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });
    
    if (site_averages.size() > n) {
        site_averages.resize(n);
    }
    return site_averages;
}

std::vector<std::pair<std::string, int>> FireRowModelService::topNSitesByMaxAQI(std::size_t n, int numThreads) const {
    // Compute max AQI per site
    std::vector<std::pair<std::string, int>> site_max_aqi;
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        std::vector<std::vector<std::pair<std::string, int>>> thread_results(omp_get_max_threads());
        
        #pragma omp parallel
        {
            int tid = omp_get_thread_num();
            #pragma omp for nowait
            for (std::size_t i = 0; i < model_->siteCount(); ++i) {
                const auto& site = model_->siteAt(i);
                if (site.measurementCount() > 0) {
                    int max_aqi = std::numeric_limits<int>::min();
                    for (std::size_t j = 0; j < site.measurementCount(); ++j) {
                        max_aqi = std::max(max_aqi, site.getMeasurement(j).aqi());
                    }
                    thread_results[tid].emplace_back(site.siteIdentifier(), max_aqi);
                }
            }
        }
        
        // Merge thread results
        for (const auto& tr : thread_results) {
            site_max_aqi.insert(site_max_aqi.end(), tr.begin(), tr.end());
        }
    } else {
        // Serial implementation
        for (std::size_t i = 0; i < model_->siteCount(); ++i) {
            const auto& site = model_->siteAt(i);
            if (site.measurementCount() > 0) {
                int max_aqi = std::numeric_limits<int>::min();
                for (std::size_t j = 0; j < site.measurementCount(); ++j) {
                    max_aqi = std::max(max_aqi, site.getMeasurement(j).aqi());
                }
                site_max_aqi.emplace_back(site.siteIdentifier(), max_aqi);
            }
        }
    }
    
    // Sort and return top N
    std::partial_sort(site_max_aqi.begin(), 
                     site_max_aqi.begin() + std::min(n, site_max_aqi.size()),
                     site_max_aqi.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });
    
    if (site_max_aqi.size() > n) {
        site_max_aqi.resize(n);
    }
    return site_max_aqi;
}

// === Category Analysis ===

std::size_t FireRowModelService::countMeasurementsByCategory(int category, int numThreads) const {
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        std::size_t count = 0;
        
        #pragma omp parallel for reduction(+:count)
        for (std::size_t i = 0; i < model_->siteCount(); ++i) {
            const auto& site = model_->siteAt(i);
            for (std::size_t j = 0; j < site.measurementCount(); ++j) {
                if (site.getMeasurement(j).category() == category) {
                    ++count;
                }
            }
        }
        return count;
    }
    
    // Serial implementation
    std::size_t count = 0;
    for (std::size_t i = 0; i < model_->siteCount(); ++i) {
        const auto& site = model_->siteAt(i);
        for (std::size_t j = 0; j < site.measurementCount(); ++j) {
            if (site.getMeasurement(j).category() == category) {
                ++count;
            }
        }
    }
    return count;
}

std::vector<std::size_t> FireRowModelService::categoryDistribution(int numThreads) const {
    std::vector<std::size_t> distribution(6, 0);
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        
        // Create thread-local distributions
        std::vector<std::vector<std::size_t>> thread_dists(omp_get_max_threads(), std::vector<std::size_t>(6, 0));
        
        #pragma omp parallel
        {
            int tid = omp_get_thread_num();
            #pragma omp for nowait
            for (std::size_t i = 0; i < model_->siteCount(); ++i) {
                const auto& site = model_->siteAt(i);
                for (std::size_t j = 0; j < site.measurementCount(); ++j) {
                    int cat = site.getMeasurement(j).category();
                    if (cat >= 0 && cat < 6) {
                        thread_dists[tid][cat]++;
                    }
                }
            }
        }
        
        // Merge thread results
        for (const auto& td : thread_dists) {
            for (int i = 0; i < 6; ++i) {
                distribution[i] += td[i];
            }
        }
    } else {
        // Serial implementation
        for (std::size_t i = 0; i < model_->siteCount(); ++i) {
            const auto& site = model_->siteAt(i);
            for (std::size_t j = 0; j < site.measurementCount(); ++j) {
                int cat = site.getMeasurement(j).category();
                if (cat >= 0 && cat < 6) {
                    distribution[cat]++;
                }
            }
        }
    }
    
    return distribution;
}


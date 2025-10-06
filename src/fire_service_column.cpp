#include "../interface/fire_service.hpp"
#include <algorithm>
#include <numeric>
#include <limits>
#include <cmath>
#include <queue>
#include <unordered_map>
#include <omp.h>

// ============================================================================
// FireColumnModelService Implementation
// ============================================================================

FireColumnModelService::FireColumnModelService(const FireColumnModel* model) : model_(model) {}
FireColumnModelService::~FireColumnModelService() = default;

std::string FireColumnModelService::getImplementationName() const {
    return "Fire Column-oriented";
}

std::size_t FireColumnModelService::totalMeasurementCount() const {
    return model_->measurementCount();
}

std::size_t FireColumnModelService::uniqueSiteCount() const {
    return model_->siteCount();
}

// === Parameter-based Aggregations ===

double FireColumnModelService::averageConcentrationForParameter(const std::string& parameter, int numThreads) const {
    auto indices = model_->getIndicesByParameter(parameter);
    if (indices.empty()) return 0.0;
    
    const auto& concentrations = model_->concentrations();
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        double total = 0.0;
        
        #pragma omp parallel for reduction(+:total)
        for (std::size_t i = 0; i < indices.size(); ++i) {
            total += concentrations[indices[i]];
        }
        return total / indices.size();
    }
    
    // Serial implementation
    double total = 0.0;
    for (std::size_t idx : indices) {
        total += concentrations[idx];
    }
    return total / indices.size();
}

double FireColumnModelService::sumConcentrationsForParameter(const std::string& parameter, int numThreads) const {
    auto indices = model_->getIndicesByParameter(parameter);
    if (indices.empty()) return 0.0;
    
    const auto& concentrations = model_->concentrations();
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        double total = 0.0;
        
        #pragma omp parallel for reduction(+:total)
        for (std::size_t i = 0; i < indices.size(); ++i) {
            total += concentrations[indices[i]];
        }
        return total;
    }
    
    // Serial implementation
    double total = 0.0;
    for (std::size_t idx : indices) {
        total += concentrations[idx];
    }
    return total;
}

double FireColumnModelService::maxConcentrationForParameter(const std::string& parameter, int numThreads) const {
    auto indices = model_->getIndicesByParameter(parameter);
    if (indices.empty()) return 0.0;
    
    const auto& concentrations = model_->concentrations();
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        double global_max = -std::numeric_limits<double>::infinity();
        
        #pragma omp parallel
        {
            double local_max = -std::numeric_limits<double>::infinity();
            #pragma omp for nowait
            for (std::size_t i = 0; i < indices.size(); ++i) {
                local_max = std::max(local_max, concentrations[indices[i]]);
            }
            #pragma omp critical
            { global_max = std::max(global_max, local_max); }
        }
        return std::isinf(global_max) ? 0.0 : global_max;
    }
    
    // Serial implementation
    double max_val = -std::numeric_limits<double>::infinity();
    for (std::size_t idx : indices) {
        max_val = std::max(max_val, concentrations[idx]);
    }
    return std::isinf(max_val) ? 0.0 : max_val;
}

double FireColumnModelService::minConcentrationForParameter(const std::string& parameter, int numThreads) const {
    auto indices = model_->getIndicesByParameter(parameter);
    if (indices.empty()) return 0.0;
    
    const auto& concentrations = model_->concentrations();
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        double global_min = std::numeric_limits<double>::infinity();
        
        #pragma omp parallel
        {
            double local_min = std::numeric_limits<double>::infinity();
            #pragma omp for nowait
            for (std::size_t i = 0; i < indices.size(); ++i) {
                local_min = std::min(local_min, concentrations[indices[i]]);
            }
            #pragma omp critical
            { global_min = std::min(global_min, local_min); }
        }
        return std::isinf(global_min) ? 0.0 : global_min;
    }
    
    // Serial implementation
    double min_val = std::numeric_limits<double>::infinity();
    for (std::size_t idx : indices) {
        min_val = std::min(min_val, concentrations[idx]);
    }
    return std::isinf(min_val) ? 0.0 : min_val;
}

// === AQI Operations ===

double FireColumnModelService::averageAQI(int numThreads) const {
    const auto& aqis = model_->aqis();
    if (aqis.empty()) return 0.0;
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        long long total = 0;
        
        #pragma omp parallel for reduction(+:total)
        for (std::size_t i = 0; i < aqis.size(); ++i) {
            total += aqis[i];
        }
        return static_cast<double>(total) / aqis.size();
    }
    
    // Serial implementation
    long long total = 0;
    for (int aqi : aqis) {
        total += aqi;
    }
    return static_cast<double>(total) / aqis.size();
}

int FireColumnModelService::maxAQI(int numThreads) const {
    const auto& aqis = model_->aqis();
    if (aqis.empty()) return 0;
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        int global_max = std::numeric_limits<int>::min();
        
        #pragma omp parallel
        {
            int local_max = std::numeric_limits<int>::min();
            #pragma omp for nowait
            for (std::size_t i = 0; i < aqis.size(); ++i) {
                local_max = std::max(local_max, aqis[i]);
            }
            #pragma omp critical
            { global_max = std::max(global_max, local_max); }
        }
        return global_max == std::numeric_limits<int>::min() ? 0 : global_max;
    }
    
    // Serial implementation
    return *std::max_element(aqis.begin(), aqis.end());
}

int FireColumnModelService::minAQI(int numThreads) const {
    const auto& aqis = model_->aqis();
    if (aqis.empty()) return 0;
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        int global_min = std::numeric_limits<int>::max();
        
        #pragma omp parallel
        {
            int local_min = std::numeric_limits<int>::max();
            #pragma omp for nowait
            for (std::size_t i = 0; i < aqis.size(); ++i) {
                local_min = std::min(local_min, aqis[i]);
            }
            #pragma omp critical
            { global_min = std::min(global_min, local_min); }
        }
        return global_min == std::numeric_limits<int>::max() ? 0 : global_min;
    }
    
    // Serial implementation
    return *std::min_element(aqis.begin(), aqis.end());
}

double FireColumnModelService::averageAQIForParameter(const std::string& parameter, int numThreads) const {
    auto indices = model_->getIndicesByParameter(parameter);
    if (indices.empty()) return 0.0;
    
    const auto& aqis = model_->aqis();
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        long long total = 0;
        
        #pragma omp parallel for reduction(+:total)
        for (std::size_t i = 0; i < indices.size(); ++i) {
            total += aqis[indices[i]];
        }
        return static_cast<double>(total) / indices.size();
    }
    
    // Serial implementation
    long long total = 0;
    for (std::size_t idx : indices) {
        total += aqis[idx];
    }
    return static_cast<double>(total) / indices.size();
}

// === Site-Specific Operations ===

double FireColumnModelService::averageConcentrationForSite(const std::string& siteName, int numThreads) const {
    auto indices = model_->getIndicesBySite(siteName);
    if (indices.empty()) return 0.0;
    
    const auto& concentrations = model_->concentrations();
    
    if (numThreads > 1 && indices.size() > 100) {
        omp_set_num_threads(numThreads);
        double total = 0.0;
        
        #pragma omp parallel for reduction(+:total)
        for (std::size_t i = 0; i < indices.size(); ++i) {
            total += concentrations[indices[i]];
        }
        return total / indices.size();
    }
    
    // Serial implementation
    double total = 0.0;
    for (std::size_t idx : indices) {
        total += concentrations[idx];
    }
    return total / indices.size();
}

std::size_t FireColumnModelService::measurementCountForSite(const std::string& siteName) const {
    return model_->getIndicesBySite(siteName).size();
}

// === Geographic Operations ===

std::size_t FireColumnModelService::countMeasurementsInBounds(double minLat, double maxLat, 
                                                              double minLon, double maxLon,
                                                              int numThreads) const {
    const auto& latitudes = model_->latitudes();
    const auto& longitudes = model_->longitudes();
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        std::size_t count = 0;
        
        #pragma omp parallel for reduction(+:count)
        for (std::size_t i = 0; i < latitudes.size(); ++i) {
            double lat = latitudes[i];
            double lon = longitudes[i];
            if (lat >= minLat && lat <= maxLat && lon >= minLon && lon <= maxLon) {
                ++count;
            }
        }
        return count;
    }
    
    // Serial implementation
    std::size_t count = 0;
    for (std::size_t i = 0; i < latitudes.size(); ++i) {
        double lat = latitudes[i];
        double lon = longitudes[i];
        if (lat >= minLat && lat <= maxLat && lon >= minLon && lon <= maxLon) {
            ++count;
        }
    }
    return count;
}

double FireColumnModelService::averageConcentrationInBounds(double minLat, double maxLat, 
                                                            double minLon, double maxLon,
                                                            int numThreads) const {
    const auto& latitudes = model_->latitudes();
    const auto& longitudes = model_->longitudes();
    const auto& concentrations = model_->concentrations();
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        double total = 0.0;
        std::size_t count = 0;
        
        #pragma omp parallel for reduction(+:total,count)
        for (std::size_t i = 0; i < latitudes.size(); ++i) {
            double lat = latitudes[i];
            double lon = longitudes[i];
            if (lat >= minLat && lat <= maxLat && lon >= minLon && lon <= maxLon) {
                total += concentrations[i];
                ++count;
            }
        }
        return count > 0 ? total / count : 0.0;
    }
    
    // Serial implementation
    double total = 0.0;
    std::size_t count = 0;
    for (std::size_t i = 0; i < latitudes.size(); ++i) {
        double lat = latitudes[i];
        double lon = longitudes[i];
        if (lat >= minLat && lat <= maxLat && lon >= minLon && lon <= maxLon) {
            total += concentrations[i];
            ++count;
        }
    }
    return count > 0 ? total / count : 0.0;
}

// === Top-N Operations ===

std::vector<std::pair<std::string, double>> FireColumnModelService::topNSitesByAverageConcentration(std::size_t n, int numThreads) const {
    const auto& uniqueSites = model_->uniqueSites();
    const auto& concentrations = model_->concentrations();
    
    std::vector<std::pair<std::string, double>> site_averages;
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        std::vector<std::vector<std::pair<std::string, double>>> thread_results(omp_get_max_threads());
        
        // Convert set to vector for iteration
        std::vector<std::string> sites_vec(uniqueSites.begin(), uniqueSites.end());
        
        #pragma omp parallel
        {
            int tid = omp_get_thread_num();
            #pragma omp for nowait
            for (std::size_t i = 0; i < sites_vec.size(); ++i) {
                auto indices = model_->getIndicesBySite(sites_vec[i]);
                if (!indices.empty()) {
                    double total = 0.0;
                    for (std::size_t idx : indices) {
                        total += concentrations[idx];
                    }
                    double avg = total / indices.size();
                    thread_results[tid].emplace_back(sites_vec[i], avg);
                }
            }
        }
        
        // Merge thread results
        for (const auto& tr : thread_results) {
            site_averages.insert(site_averages.end(), tr.begin(), tr.end());
        }
    } else {
        // Serial implementation
        for (const auto& site : uniqueSites) {
            auto indices = model_->getIndicesBySite(site);
            if (!indices.empty()) {
                double total = 0.0;
                for (std::size_t idx : indices) {
                    total += concentrations[idx];
                }
                double avg = total / indices.size();
                site_averages.emplace_back(site, avg);
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

std::vector<std::pair<std::string, int>> FireColumnModelService::topNSitesByMaxAQI(std::size_t n, int numThreads) const {
    const auto& uniqueSites = model_->uniqueSites();
    const auto& aqis = model_->aqis();
    
    std::vector<std::pair<std::string, int>> site_max_aqi;
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        std::vector<std::vector<std::pair<std::string, int>>> thread_results(omp_get_max_threads());
        
        // Convert set to vector for iteration
        std::vector<std::string> sites_vec(uniqueSites.begin(), uniqueSites.end());
        
        #pragma omp parallel
        {
            int tid = omp_get_thread_num();
            #pragma omp for nowait
            for (std::size_t i = 0; i < sites_vec.size(); ++i) {
                auto indices = model_->getIndicesBySite(sites_vec[i]);
                if (!indices.empty()) {
                    int max_aqi = std::numeric_limits<int>::min();
                    for (std::size_t idx : indices) {
                        max_aqi = std::max(max_aqi, aqis[idx]);
                    }
                    thread_results[tid].emplace_back(sites_vec[i], max_aqi);
                }
            }
        }
        
        // Merge thread results
        for (const auto& tr : thread_results) {
            site_max_aqi.insert(site_max_aqi.end(), tr.begin(), tr.end());
        }
    } else {
        // Serial implementation
        for (const auto& site : uniqueSites) {
            auto indices = model_->getIndicesBySite(site);
            if (!indices.empty()) {
                int max_aqi = std::numeric_limits<int>::min();
                for (std::size_t idx : indices) {
                    max_aqi = std::max(max_aqi, aqis[idx]);
                }
                site_max_aqi.emplace_back(site, max_aqi);
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

std::size_t FireColumnModelService::countMeasurementsByCategory(int category, int numThreads) const {
    const auto& categories = model_->categories();
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        std::size_t count = 0;
        
        #pragma omp parallel for reduction(+:count)
        for (std::size_t i = 0; i < categories.size(); ++i) {
            if (categories[i] == category) {
                ++count;
            }
        }
        return count;
    }
    
    // Serial implementation
    return std::count(categories.begin(), categories.end(), category);
}

std::vector<std::size_t> FireColumnModelService::categoryDistribution(int numThreads) const {
    const auto& categories = model_->categories();
    std::vector<std::size_t> distribution(6, 0);
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        
        // Create thread-local distributions
        std::vector<std::vector<std::size_t>> thread_dists(omp_get_max_threads(), std::vector<std::size_t>(6, 0));
        
        #pragma omp parallel
        {
            int tid = omp_get_thread_num();
            #pragma omp for nowait
            for (std::size_t i = 0; i < categories.size(); ++i) {
                int cat = categories[i];
                if (cat >= 0 && cat < 6) {
                    thread_dists[tid][cat]++;
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
        for (int cat : categories) {
            if (cat >= 0 && cat < 6) {
                distribution[cat]++;
            }
        }
    }
    
    return distribution;
}


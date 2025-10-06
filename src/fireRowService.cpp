#include "../interface/fire_service_direct.hpp"
#include "../interface/fireRowModel.hpp"
#include <algorithm>
#include <numeric>
#include <queue>
#include <functional>
#include <omp.h>
#include <limits>

FireRowService::FireRowService(const FireRowModel* model) : model_(model) {}
FireRowService::~FireRowService() = default;

std::string FireRowService::getImplementationName() const {
    return "Fire Row-oriented";
}

std::size_t FireRowService::totalMeasurementCount() const {
    return model_->totalMeasurements();
}

std::size_t FireRowService::uniqueSiteCount() const {
    return model_->siteCount();
}

int FireRowService::maxAQI(int numThreads) const {
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        int global_max = std::numeric_limits<int>::min();
        
#pragma omp parallel
        {
            int local_max = std::numeric_limits<int>::min();
#pragma omp for nowait
            for (std::size_t i = 0; i < model_->siteCount(); ++i) {
                const FireSiteData& site = model_->siteAt(i);
                for (const auto& measurement : site.measurements()) {
                    local_max = std::max(local_max, measurement.aqi());
                }
            }
#pragma omp critical
            {
                global_max = std::max(global_max, local_max);
            }
        }
        return global_max == std::numeric_limits<int>::min() ? 0 : global_max;
    }
    
    // Serial version
    int maxAQIValue = 0;
    for (std::size_t i = 0; i < model_->siteCount(); ++i) {
        const FireSiteData& site = model_->siteAt(i);
        for (const auto& measurement : site.measurements()) {
            maxAQIValue = std::max(maxAQIValue, measurement.aqi());
        }
    }
    return maxAQIValue;
}

int FireRowService::minAQI(int numThreads) const {
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        int global_min = std::numeric_limits<int>::max();
        
#pragma omp parallel
        {
            int local_min = std::numeric_limits<int>::max();
#pragma omp for nowait
            for (std::size_t i = 0; i < model_->siteCount(); ++i) {
                const FireSiteData& site = model_->siteAt(i);
                for (const auto& measurement : site.measurements()) {
                    if (measurement.aqi() > 0) { // Only consider valid AQI values
                        local_min = std::min(local_min, measurement.aqi());
                    }
                }
            }
#pragma omp critical
            {
                global_min = std::min(global_min, local_min);
            }
        }
        return global_min == std::numeric_limits<int>::max() ? 0 : global_min;
    }
    
    // Serial version
    int minAQIValue = std::numeric_limits<int>::max();
    for (std::size_t i = 0; i < model_->siteCount(); ++i) {
        const FireSiteData& site = model_->siteAt(i);
        for (const auto& measurement : site.measurements()) {
            if (measurement.aqi() > 0) { // Only consider valid AQI values
                minAQIValue = std::min(minAQIValue, measurement.aqi());
            }
        }
    }
    return minAQIValue == std::numeric_limits<int>::max() ? 0 : minAQIValue;
}

double FireRowService::averageAQI(int numThreads) const {
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        long long total = 0;
        long long count = 0;
        
#pragma omp parallel for reduction(+:total, count)
        for (std::size_t i = 0; i < model_->siteCount(); ++i) {
            const FireSiteData& site = model_->siteAt(i);
            for (const auto& measurement : site.measurements()) {
                total += measurement.aqi();
                ++count;
            }
        }
        return count > 0 ? static_cast<double>(total) / static_cast<double>(count) : 0.0;
    }
    
    // Serial version
    long long total = 0;
    std::size_t count = 0;
    for (std::size_t i = 0; i < model_->siteCount(); ++i) {
        const FireSiteData& site = model_->siteAt(i);
        for (const auto& measurement : site.measurements()) {
            total += measurement.aqi();
            ++count;
        }
    }
    return count > 0 ? static_cast<double>(total) / count : 0.0;
}

std::vector<std::pair<std::string, double>> FireRowService::topNSitesByAverageConcentration(std::size_t n, int numThreads) const {
    if (n == 0) return {};
    
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        
        // Define heap element and min-heap type for top-N selection
        using HeapElem = std::pair<double, std::string>; // (avg_concentration, site_name)
        using MinHeap = std::priority_queue<HeapElem, std::vector<HeapElem>, std::greater<HeapElem>>;
        
        int threads = omp_get_max_threads();
        std::vector<MinHeap> localHeaps(static_cast<std::size_t>(threads)); // one heap per thread
        
#pragma omp parallel
        {
            int tid = omp_get_thread_num();
            MinHeap &heap = localHeaps[static_cast<std::size_t>(tid)];
#pragma omp for nowait
            for (std::size_t i = 0; i < model_->siteCount(); ++i) {
                const FireSiteData& site = model_->siteAt(i);
                if (site.measurementCount() == 0) continue;
                
                // Calculate average concentration for this site
                double totalConcentration = 0.0;
                std::size_t measurementCount = 0;
                for (const auto& measurement : site.measurements()) {
                    totalConcentration += measurement.concentration();
                    ++measurementCount;
                }
                
                if (measurementCount > 0) {
                    double avgConcentration = totalConcentration / measurementCount;
                    HeapElem e{avgConcentration, site.siteIdentifier()};
                    
                    // Maintain top-N in each thread's heap
                    if (heap.size() < n) {
                        heap.push(e);
                    } else if (e > heap.top()) {
                        heap.pop();
                        heap.push(e);
                    }
                }
            }
        }
        
        // Merge all thread-local heaps into a single final heap of size up to n
        MinHeap finalHeap;
        for (auto &h : localHeaps) {
            while (!h.empty()) {
                HeapElem e = h.top(); h.pop();
                if (finalHeap.size() < n) {
                    finalHeap.push(e);
                } else if (e > finalHeap.top()) {
                    finalHeap.pop();
                    finalHeap.push(e);
                }
            }
        }
        
        // Extract results from finalHeap into vector sorted descending by concentration
        std::vector<std::pair<std::string, double>> out;
        out.reserve(finalHeap.size());
        while (!finalHeap.empty()) {
            auto p = finalHeap.top(); finalHeap.pop(); // smallest-first
            out.emplace_back(p.second, p.first); // (site_name, avg_concentration)
        }
        std::reverse(out.begin(), out.end()); // now descending
        return out;
    }
    
    // Serial version: collect all site average concentrations
    std::vector<std::pair<std::string, double>> siteAvgConcentrations;
    for (std::size_t i = 0; i < model_->siteCount(); ++i) {
        const FireSiteData& site = model_->siteAt(i);
        if (site.measurementCount() == 0) continue;
        
        // Calculate average concentration for this site
        double totalConcentration = 0.0;
        std::size_t measurementCount = 0;
        for (const auto& measurement : site.measurements()) {
            totalConcentration += measurement.concentration();
            ++measurementCount;
        }
        
        if (measurementCount > 0) {
            double avgConcentration = totalConcentration / measurementCount;
            siteAvgConcentrations.emplace_back(site.siteIdentifier(), avgConcentration);
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
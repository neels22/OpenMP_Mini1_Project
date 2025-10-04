/**
 * @file service_column.cpp
 * @brief Column-oriented population model service implementation
 * 
 * This file implements analytics operations optimized for column-oriented data layout.
 * The column-oriented approach stores all countries' data for each year contiguously,
 * providing superior cache locality and vectorization opportunities for per-year
 * aggregations and analytics operations.
 * 
 * Key Optimizations:
 * - Direct indexing for O(1) country-year access
 * - Contiguous memory access patterns for better cache performance
 * - OpenMP parallel reductions over cache-friendly data layout
 * - Per-thread min-heap optimization for top-N operations
 */

#include "../interface/service.hpp"
#include "../interface/populationModelColumn.hpp"
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <chrono>
#include <queue>
#include <functional>
#include <omp.h>

PopulationModelColumnService::PopulationModelColumnService(PopulationModelColumn* m) : model_(m) {}
PopulationModelColumnService::~PopulationModelColumnService() = default;

std::string PopulationModelColumnService::getImplementationName() const {
    return "Column-oriented";
}

long long PopulationModelColumnService::sumPopulationForYear(int year, int numThreads) const {
    // Find year index with O(1) hash map lookup
    const auto& yearMap = model_->yearToIndex();
    auto it = yearMap.find(year);
    if (it == yearMap.end()) return 0;
    std::size_t yearIndex = static_cast<std::size_t>(it->second);
    
    long long total = 0;
    std::size_t rows = model_->rowCount();
    
    if (numThreads > 1) {
        // Parallel reduction over contiguous memory for optimal cache usage
        omp_set_num_threads(numThreads);
#pragma omp parallel for reduction(+:total)
        for (std::size_t i = 0; i < rows; ++i) {
            total += model_->getPopulationForCountryYear(i, yearIndex);
        }
        return total;
    }
    
    // Serial version for comparison - same access pattern
    for (std::size_t i = 0; i < rows; ++i) {
        total += model_->getPopulationForCountryYear(i, yearIndex);
    }
    return total;
}

double PopulationModelColumnService::averagePopulationForYear(int year, int numThreads) const {
    // Leverage sum calculation and divide by count for consistency
    const auto& yearMap = model_->yearToIndex();
    auto it = yearMap.find(year);
    if (it == yearMap.end()) return 0.0;
    std::size_t yearIndex = static_cast<std::size_t>(it->second);
    
    long long total = 0;
    std::size_t rows = model_->rowCount();
    
    if (numThreads > 1) {
        // Parallel reduction with same pattern as sum for consistency
        omp_set_num_threads(numThreads);
#pragma omp parallel for reduction(+:total)
        for (std::size_t i = 0; i < rows; ++i) {
            total += model_->getPopulationForCountryYear(i, yearIndex);
        }
        return rows > 0 ? static_cast<double>(total) / static_cast<double>(rows) : 0.0;
    }
    
    // Serial calculation
    for (std::size_t i = 0; i < rows; ++i) {
        total += model_->getPopulationForCountryYear(i, yearIndex);
    }
    return rows > 0 ? static_cast<double>(total) / static_cast<double>(rows) : 0.0;
}

long long PopulationModelColumnService::maxPopulationForYear(int year, int numThreads) const {
    const auto& yearMap = model_->yearToIndex();
    auto it = yearMap.find(year);
    if (it == yearMap.end()) return 0;
    std::size_t yearIndex = static_cast<std::size_t>(it->second);
    std::size_t rows = model_->rowCount();
    long long global_max = std::numeric_limits<long long>::min();
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
#pragma omp parallel
        {
            long long local_max = std::numeric_limits<long long>::min();
#pragma omp for nowait
            for (std::size_t i = 0; i < rows; ++i) local_max = std::max(local_max, model_->getPopulationForCountryYear(i, yearIndex));
#pragma omp critical
            { global_max = std::max(global_max, local_max); }
        }
        return global_max == std::numeric_limits<long long>::min() ? 0 : global_max;
    }
    for (std::size_t i = 0; i < rows; ++i) global_max = std::max(global_max, model_->getPopulationForCountryYear(i, yearIndex));
    return global_max == std::numeric_limits<long long>::min() ? 0 : global_max;
}

long long PopulationModelColumnService::minPopulationForYear(int year, int numThreads) const {
    const auto& yearMap = model_->yearToIndex();
    auto it = yearMap.find(year);
    if (it == yearMap.end()) return 0;
    std::size_t yearIndex = static_cast<std::size_t>(it->second);
    std::size_t rows = model_->rowCount();
    long long global_min = std::numeric_limits<long long>::max();
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
#pragma omp parallel
        {
            long long local_min = std::numeric_limits<long long>::max();
#pragma omp for nowait
            for (std::size_t i = 0; i < rows; ++i) local_min = std::min(local_min, model_->getPopulationForCountryYear(i, yearIndex));
#pragma omp critical
            { global_min = std::min(global_min, local_min); }
        }
        return global_min == std::numeric_limits<long long>::max() ? 0 : global_min;
    }
    for (std::size_t i = 0; i < rows; ++i) global_min = std::min(global_min, model_->getPopulationForCountryYear(i, yearIndex));
    return global_min == std::numeric_limits<long long>::max() ? 0 : global_min;
}

long long PopulationModelColumnService::populationForCountryInYear(const std::string& country, int year, int numThreads) const {
    (void)numThreads;
    const auto& yearMap = model_->yearToIndex();
    auto it = yearMap.find(year);
    if (it == yearMap.end()) return 0;
    std::size_t yearIndex = static_cast<std::size_t>(it->second);
    int cidx = model_->countryNameIndex(country);
    if (cidx < 0) return 0;
    return model_->getPopulationForCountryYear(static_cast<std::size_t>(cidx), yearIndex);
}

std::vector<std::pair<std::string, long long>> PopulationModelColumnService::topNCountriesByPopulationInYear(int year, std::size_t n, int numThreads) const {
    if (n == 0) return {};
    const auto& yearMap = model_->yearToIndex();
    auto it = yearMap.find(year);
    if (it == yearMap.end()) return {};
    std::size_t yearIndex = static_cast<std::size_t>(it->second);
    std::size_t rows = model_->rowCount();

    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        using HeapElem = std::pair<long long, std::string>;
        using MinHeap = std::priority_queue<HeapElem, std::vector<HeapElem>, std::greater<HeapElem>>;

        int threads = omp_get_max_threads();
        std::vector<MinHeap> localHeaps(static_cast<std::size_t>(threads));

#pragma omp parallel
        {
            int tid = omp_get_thread_num();
            MinHeap &heap = localHeaps[static_cast<std::size_t>(tid)];
#pragma omp for nowait
            for (std::size_t i = 0; i < rows; ++i) {
                long long val = model_->getPopulationForCountryYear(i, yearIndex);
                HeapElem e{val, model_->countryNames()[i]};
                if (heap.size() < n) heap.push(e);
                else if (e > heap.top()) { heap.pop(); heap.push(e); }
            }
        }

        MinHeap finalHeap;
        for (auto &h : localHeaps) {
            while (!h.empty()) {
                auto e = h.top(); h.pop();
                if (finalHeap.size() < n) finalHeap.push(e);
                else if (e > finalHeap.top()) { finalHeap.pop(); finalHeap.push(e); }
            }
        }

        std::vector<std::pair<std::string,long long>> out;
        out.reserve(finalHeap.size());
        while (!finalHeap.empty()) {
            auto p = finalHeap.top(); finalHeap.pop();
            out.emplace_back(p.second, p.first);
        }
        std::reverse(out.begin(), out.end());
        return out;
    }

    std::vector<std::pair<std::string,long long>> out;
    out.reserve(rows);
    for (std::size_t i = 0; i < rows; ++i) out.emplace_back(model_->countryNames()[i], model_->getPopulationForCountryYear(i, yearIndex));
    std::sort(out.begin(), out.end(), [](const auto &a, const auto &b){ return a.second > b.second; });
    if (out.size() > n) out.resize(n);
    return out;
}

std::vector<long long> PopulationModelColumnService::populationOverYearsForCountry(const std::string& country, int startYear, int endYear, int numThreads) const {
    (void)numThreads;
    const auto& yearMap = model_->yearToIndex();
    auto itStart = yearMap.find(startYear);
    auto itEnd = yearMap.find(endYear);
    if (itStart == yearMap.end() || itEnd == yearMap.end()) return {};
    std::size_t startIndex = static_cast<std::size_t>(itStart->second);
    std::size_t endIndex = static_cast<std::size_t>(itEnd->second);
    int cidx = model_->countryNameIndex(country);
    if (cidx < 0) return {};
    std::vector<long long> res;
    for (std::size_t y = startIndex; y <= endIndex; ++y) res.push_back(model_->getPopulationForCountryYear(static_cast<std::size_t>(cidx), y));
    return res;
}

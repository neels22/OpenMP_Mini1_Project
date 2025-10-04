#include "../interface/service.hpp"
#include "../interface/populationModel.hpp"
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <chrono>
#include <queue>
#include <functional>

#include <omp.h>

PopulationModelService::PopulationModelService(PopulationModel* m) : model_(m) {}
PopulationModelService::~PopulationModelService() = default;

long long PopulationModelService::sumPopulationForYear(int year, int numThreads) const {
    // If caller requests multiple threads and OpenMP is available, delegate to parallel implementation
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        const auto& yearMap = model_->yearToIndex();
        auto it = yearMap.find(year);
        if (it == yearMap.end()) return 0;
    std::size_t yearIndex = static_cast<std::size_t>(it->second);
        long long total = 0;
#pragma omp parallel for reduction(+:total)
        for (std::size_t i = 0; i < model_->rowCount(); ++i) {
            const PopulationRow& row = model_->rowAt(i);
            if (yearIndex < row.yearCount()) total += row.getPopulationForYear(yearIndex);
        }
        return total;
    }
    const auto& yearMap = model_->yearToIndex();
    auto it = yearMap.find(year);
    if (it == yearMap.end()) return 0;
    std::size_t yearIndex = static_cast<std::size_t>(it->second);
    long long total = 0;
    for (std::size_t i = 0; i < model_->rowCount(); ++i) {
        const PopulationRow& row = model_->rowAt(i);
        if (yearIndex < row.yearCount()) total += row.getPopulationForYear(yearIndex);
    }
    return total;
}

double PopulationModelService::averagePopulationForYear(int year, int numThreads) const {
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        const auto& yearMap = model_->yearToIndex();
        auto it = yearMap.find(year);
        if (it == yearMap.end()) return 0.0;
    std::size_t yearIndex = static_cast<std::size_t>(it->second);
        long long total = 0;
        long long countLL = 0;
#pragma omp parallel for reduction(+:total, countLL)
        for (std::size_t i = 0; i < model_->rowCount(); ++i) {
            const PopulationRow& row = model_->rowAt(i);
            if (yearIndex < row.yearCount()) { total += row.getPopulationForYear(yearIndex); ++countLL; }
        }
        return countLL > 0 ? static_cast<double>(total) / static_cast<double>(countLL) : 0.0;
    }
    const auto& yearMap = model_->yearToIndex();
    auto it = yearMap.find(year);
    if (it == yearMap.end()) return 0.0;
    std::size_t yearIndex = static_cast<std::size_t>(it->second);
    long long total = 0;
    std::size_t count = 0;
    for (std::size_t i = 0; i < model_->rowCount(); ++i) {
        const PopulationRow& row = model_->rowAt(i);
        if (yearIndex < row.yearCount()) { total += row.getPopulationForYear(yearIndex); ++count; }
    }
    return count > 0 ? static_cast<double>(total) / count : 0.0;
}

// Parallel helpers removed: parallel code is inlined in the numThreads>1 branches.

long long PopulationModelService::maxPopulationForYear(int year, int numThreads) const {
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        const auto& yearMap = model_->yearToIndex();
        auto it = yearMap.find(year);
        if (it == yearMap.end()) return 0;
    std::size_t yearIndex = static_cast<std::size_t>(it->second);
        long long global_max = std::numeric_limits<long long>::min();
#pragma omp parallel
        {
            long long local_max = std::numeric_limits<long long>::min();
#pragma omp for nowait
            for (std::size_t i = 0; i < model_->rowCount(); ++i) {
                const PopulationRow& row = model_->rowAt(i);
                if (yearIndex < row.yearCount()) local_max = std::max(local_max, row.getPopulationForYear(yearIndex));
            }
#pragma omp critical
            { global_max = std::max(global_max, local_max); }
        }
        return global_max == std::numeric_limits<long long>::min() ? 0 : global_max;
    }
    const auto& yearMap = model_->yearToIndex();
    auto it = yearMap.find(year);
    if (it == yearMap.end()) return 0;
    std::size_t yearIndex = static_cast<std::size_t>(it->second);
    long long maxPop = 0;
    for (std::size_t i = 0; i < model_->rowCount(); ++i) {
        const PopulationRow& row = model_->rowAt(i);
        if (yearIndex < row.yearCount()) maxPop = std::max(maxPop, row.getPopulationForYear(yearIndex));
    }
    return maxPop;
}

// Parallel helpers removed: parallel code is inlined in the numThreads>1 branches.

long long PopulationModelService::minPopulationForYear(int year, int numThreads) const {
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        const auto& yearMap = model_->yearToIndex();
        auto it = yearMap.find(year);
        if (it == yearMap.end()) return 0;
    std::size_t yearIndex = static_cast<std::size_t>(it->second);
        long long global_min = std::numeric_limits<long long>::max();
#pragma omp parallel
        {
            long long local_min = std::numeric_limits<long long>::max();
#pragma omp for nowait
            for (std::size_t i = 0; i < model_->rowCount(); ++i) {
                const PopulationRow& row = model_->rowAt(i);
                if (yearIndex < row.yearCount()) local_min = std::min(local_min, row.getPopulationForYear(yearIndex));
            }
#pragma omp critical
            { global_min = std::min(global_min, local_min); }
        }
        return global_min == std::numeric_limits<long long>::max() ? 0 : global_min;
    }
    const auto& yearMap = model_->yearToIndex();
    auto it = yearMap.find(year);
    if (it == yearMap.end()) return 0;
    std::size_t yearIndex = static_cast<std::size_t>(it->second);
    long long minPop = std::numeric_limits<long long>::max();
    for (std::size_t i = 0; i < model_->rowCount(); ++i) {
        const PopulationRow& row = model_->rowAt(i);
        if (yearIndex < row.yearCount()) minPop = std::min(minPop, row.getPopulationForYear(yearIndex));
    }
    return minPop == std::numeric_limits<long long>::max() ? 0 : minPop;
}

// Parallel helpers removed: parallel code is inlined in the numThreads>1 branches.

long long PopulationModelService::populationForCountryInYear(const std::string& country, int year, int numThreads) const {
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        // Per-country lookup is effectively O(1) via hash -> just perform serial lookup even when asking for multiple threads
        (void)country; (void)year; // silence unused warnings in some builds
        // fall through to serial implementation below
    }
    const PopulationRow* row = model_->getByCountry(country);
    if (!row) return 0;
    const auto& yearMap = model_->yearToIndex();
    auto it = yearMap.find(year);
    if (it == yearMap.end()) return 0;
    std::size_t yearIndex = static_cast<std::size_t>(it->second);
    if (yearIndex >= row->yearCount()) return 0;
    return static_cast<long long>(row->getPopulationForYear(yearIndex));
}

std::vector<std::pair<std::string, long long>> PopulationModelService::topNCountriesByPopulationInYear(int year, std::size_t n, int numThreads) const {
    if (n == 0) return {};
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        const auto& yearMap = model_->yearToIndex();
        auto it = yearMap.find(year);
        if (it == yearMap.end()) return {};
        std::size_t yearIndex = static_cast<std::size_t>(it->second);

        using HeapElem = std::pair<long long, std::string>; // (population, country)
        using MinHeap = std::priority_queue<HeapElem, std::vector<HeapElem>, std::greater<HeapElem>>;

        int threads = omp_get_max_threads();
        std::vector<MinHeap> localHeaps(static_cast<std::size_t>(threads));

#pragma omp parallel
        {
            int tid = omp_get_thread_num();
            MinHeap &heap = localHeaps[static_cast<std::size_t>(tid)];
#pragma omp for nowait
            for (std::size_t i = 0; i < model_->rowCount(); ++i) {
                const PopulationRow& row = model_->rowAt(i);
                if (yearIndex >= row.yearCount()) continue;
                HeapElem e{row.getPopulationForYear(yearIndex), row.country()};
                if (heap.size() < n) heap.push(e);
                else if (e > heap.top()) { heap.pop(); heap.push(e); }
            }
        }

        // Merge per-thread heaps into a final min-heap of size up to n
        MinHeap finalHeap;
        for (auto &h : localHeaps) {
            while (!h.empty()) {
                HeapElem e = h.top(); h.pop();
                if (finalHeap.size() < n) finalHeap.push(e);
                else if (e > finalHeap.top()) { finalHeap.pop(); finalHeap.push(e); }
            }
        }

        // Extract results from finalHeap into vector sorted descending
        std::vector<std::pair<std::string,long long>> out;
        out.reserve(finalHeap.size());
        while (!finalHeap.empty()) {
            auto p = finalHeap.top(); finalHeap.pop(); // smallest-first
            out.emplace_back(p.second, p.first);
        }
        std::reverse(out.begin(), out.end()); // now descending
        return out;
    }
    const auto& yearMap = model_->yearToIndex();
    auto it = yearMap.find(year);
    if (it == yearMap.end()) return {};
    std::size_t yearIndex = static_cast<std::size_t>(it->second);
    std::vector<std::pair<std::string, long long>> countryPops;
    for (std::size_t i = 0; i < model_->rowCount(); ++i) {
        const PopulationRow& row = model_->rowAt(i);
        if (yearIndex < row.yearCount()) countryPops.emplace_back(row.country(), row.getPopulationForYear(yearIndex));
    }
    std::sort(countryPops.begin(), countryPops.end(), [](const auto& a, const auto& b){ return a.second > b.second; });
    if (countryPops.size() > n) countryPops.resize(n);
    return countryPops;
}

// Parallel helpers removed: parallel code is inlined in the numThreads>1 branches.

std::vector<long long> PopulationModelService::populationOverYearsForCountry(const std::string& country, int startYear, int endYear, int numThreads) const {
    if (numThreads > 1) {
        omp_set_num_threads(numThreads);
        // per-country over-years is small; run serially even when numThreads>1
        (void)country; (void)startYear; (void)endYear;
        // fall through to serial implementation below
    }
    const PopulationRow* row = model_->getByCountry(country);
    if (!row) return {};
    const auto& yearMap = model_->yearToIndex();
    auto itStart = yearMap.find(startYear);
    auto itEnd = yearMap.find(endYear);
    if (itStart == yearMap.end() || itEnd == yearMap.end()) return {};
    std::size_t startIndex = static_cast<std::size_t>(itStart->second);
    std::size_t endIndex = static_cast<std::size_t>(itEnd->second);
    if (startIndex >= row->yearCount() || endIndex >= row->yearCount() || startIndex > endIndex) return {};
    std::vector<long long> populations;
    for (std::size_t i = startIndex; i <= endIndex; ++i) populations.push_back(row->getPopulationForYear(i));
    return populations;
}


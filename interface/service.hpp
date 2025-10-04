#pragma once

#include "populationModel.hpp"
#include <vector>
#include <string>
#include <utility>

class PopulationModelService {
public:
    explicit PopulationModelService(PopulationModel* m);
    ~PopulationModelService();

    // Computations (single API: pass numThreads>1 to request multi-threaded execution)
    long long sumPopulationForYear(int year, int numThreads = 1) const;
    double averagePopulationForYear(int year, int numThreads = 1) const;
    long long maxPopulationForYear(int year, int numThreads = 1) const;
    long long minPopulationForYear(int year, int numThreads = 1) const;
    long long populationForCountryInYear(const std::string& country, int year, int numThreads = 1) const;
    std::vector<std::pair<std::string, long long>> topNCountriesByPopulationInYear(int year, std::size_t n, int numThreads = 1) const;
    std::vector<long long> poputationOverYearsForCountry(const std::string& country,int startYear, int endYear, int numThreads = 1) const;

private:
    PopulationModel* model_;
};

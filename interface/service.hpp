#pragma once

#include "populationModel.hpp"
#include "populationModelColumn.hpp"
#include "population_service_interface.hpp"
#include <vector>
#include <string>
#include <utility>

/**
 * @file service.hpp
 * @brief Service layer providing unified analytics APIs for both row and column data models
 * 
 * This file defines concrete service classes that implement the IPopulationService interface
 * for both row-oriented and column-oriented population models. This allows for fair
 * performance comparisons between different data layouts while maintaining
 * clean separation between data storage and business logic.
 */

/**
 * @class PopulationModelService
 * @brief Service layer for row-oriented population data analytics
 * 
 * Concrete implementation of IPopulationService for row-oriented population data.
 * All operations support both serial and parallel execution modes through the
 * numThreads parameter. The service acts as a facade over the PopulationModel,
 * implementing complex queries and aggregations with OpenMP parallelization.
 * 
 * Key Features:
 * - Implements IPopulationService interface
 * - OpenMP-optimized algorithms for parallel operations
 * - Per-thread min-heap optimization for top-N queries
 * - Efficient reduction patterns for aggregations
 */
class PopulationModelService : public IPopulationService {
public:
    /// Constructor takes ownership of model pointer (non-owning)
    explicit PopulationModelService(PopulationModel* m);
    
    /// Destructor - model cleanup is handled externally
    ~PopulationModelService() override;

    // === IPopulationService Implementation ===
    
    long long sumPopulationForYear(int year, int numThreads = 1) const override;
    double averagePopulationForYear(int year, int numThreads = 1) const override;
    long long maxPopulationForYear(int year, int numThreads = 1) const override;
    long long minPopulationForYear(int year, int numThreads = 1) const override;
    long long populationForCountryInYear(const std::string& country, int year, int numThreads = 1) const override;
    std::vector<long long> populationOverYearsForCountry(const std::string& country, int startYear, int endYear, int numThreads = 1) const override;
    std::vector<std::pair<std::string, long long>> topNCountriesByPopulationInYear(int year, std::size_t n, int numThreads = 1) const override;
    std::string getImplementationName() const override;

private:
    PopulationModel* model_;  ///< Non-owning pointer to underlying data model
};

/**
 * @class PopulationModelColumnService
 * @brief Service layer for column-oriented population data analytics
 * 
 * Concrete implementation of IPopulationService for column-oriented population data.
 * Provides identical interface to PopulationModelService but operates on column-oriented
 * data. This enables direct performance comparisons between row and column layouts
 * while using the same business logic and API contracts.
 * 
 * The column-oriented implementation typically provides superior performance for:
 * - Per-year aggregations (sum, avg, min, max)
 * - Operations that scan many countries
 * - Parallel reductions over contiguous memory
 * 
 * Key Features:
 * - Implements IPopulationService interface
 * - Optimized for columnar data access patterns
 * - Superior cache locality for per-year operations
 * - Efficient vectorization opportunities
 */
class PopulationModelColumnService : public IPopulationService {
public:
    /// Constructor takes ownership of model pointer (non-owning)
    explicit PopulationModelColumnService(PopulationModelColumn* m);
    
    /// Destructor - model cleanup is handled externally
    ~PopulationModelColumnService() override;

    // === IPopulationService Implementation ===
    
    long long sumPopulationForYear(int year, int numThreads = 1) const override;
    double averagePopulationForYear(int year, int numThreads = 1) const override;
    long long maxPopulationForYear(int year, int numThreads = 1) const override;
    long long minPopulationForYear(int year, int numThreads = 1) const override;
    long long populationForCountryInYear(const std::string& country, int year, int numThreads = 1) const override;
    std::vector<long long> populationOverYearsForCountry(const std::string& country, int startYear, int endYear, int numThreads = 1) const override;
    std::vector<std::pair<std::string, long long>> topNCountriesByPopulationInYear(int year, std::size_t n, int numThreads = 1) const override;
    std::string getImplementationName() const override;

private:
    PopulationModelColumn* model_;  ///< Non-owning pointer to underlying columnar data model
};


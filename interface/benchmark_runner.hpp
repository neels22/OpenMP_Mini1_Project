#pragma once

#include "population_service_interface.hpp"
#include "benchmark_utils.hpp"
#include <vector>
#include <string>
#include <functional>
#include <iostream>
#include <iomanip>

/**
 * @file benchmark_runner.hpp
 * @brief Generic benchmark runner for population services
 * 
 * This module provides generic benchmark templates that work with any implementation
 * of IPopulationService, eliminating code duplication between row and column benchmarks.
 * The templates automatically handle both serial and parallel execution, result validation,
 * and performance reporting.
 */

namespace BenchmarkRunner {

    /**
     * @struct BenchmarkConfig
     * @brief Configuration for benchmark execution
     */
    struct BenchmarkConfig {
        int parallelThreads = 4;      ///< Number of threads for parallel execution
        int repetitions = 5;          ///< Number of benchmark repetitions
        bool validateResults = true;  ///< Whether to validate serial vs parallel results
        bool showValues = true;       ///< Whether to display result values
    };

    /**
     * @brief Run aggregation benchmark for a single operation
     * 
     * Generic template that benchmarks any aggregation operation (sum, avg, min, max)
     * against both implementations, automatically handling serial/parallel execution
     * and result validation.
     * 
     * @tparam T Return type of the operation
     * @param services Vector of service implementations to benchmark
     * @param operationName Human-readable name of the operation
     * @param operation Lambda/function that takes (service, numThreads) and returns T
     * @param year Target year for the operation
     * @param config Benchmark configuration
     */
    template<typename T>
    void runAggregationBenchmark(
        const std::vector<std::reference_wrapper<IPopulationService>>& services,
        const std::string& operationName,
        std::function<T(const IPopulationService&, int)> operation,
        int year,
        const BenchmarkConfig& config = {});

    /**
     * @brief Run top-N benchmark for ranking operations
     * 
     * Specialized benchmark for top-N operations that return vectors of pairs.
     * Validates result count consistency and optionally shows sample results.
     * 
     * @param services Vector of service implementations to benchmark
     * @param operationName Human-readable name of the operation
     * @param year Target year for ranking
     * @param n Number of top results to return
     * @param config Benchmark configuration
     */
    void runTopNBenchmark(
        const std::vector<std::reference_wrapper<IPopulationService>>& services,
        const std::string& operationName,
        int year,
        std::size_t n,
        const BenchmarkConfig& config = {});

    /**
     * @brief Run country-specific benchmark
     * 
     * Benchmarks operations that operate on specific countries, such as
     * populationForCountryInYear or populationOverYearsForCountry.
     * 
     * @tparam T Return type of the operation
     * @param services Vector of service implementations to benchmark
     * @param operationName Human-readable name of the operation
     * @param operation Lambda/function that takes (service, country, ..., numThreads)
     * @param country Target country name
     * @param config Benchmark configuration
     */
    template<typename T>
    void runCountryBenchmark(
        const std::vector<std::reference_wrapper<IPopulationService>>& services,
        const std::string& operationName,
        std::function<T(const IPopulationService&, const std::string&, int)> operation,
        const std::string& country,
        const BenchmarkConfig& config = {});

    /**
     * @brief Run year range benchmark for country time series
     * 
     * Specialized benchmark for populationOverYearsForCountry operation.
     * 
     * @param services Vector of service implementations to benchmark
     * @param country Target country name
     * @param startYear Start of year range
     * @param endYear End of year range
     * @param config Benchmark configuration
     */
    void runYearRangeBenchmark(
        const std::vector<std::reference_wrapper<IPopulationService>>& services,
        const std::string& country,
        int startYear,
        int endYear,
        const BenchmarkConfig& config = {});

    /**
     * @brief Run comprehensive benchmark suite
     * 
     * Runs all standard benchmarks (aggregations, top-N, country-specific)
     * across all provided service implementations with a single function call.
     * 
     * @param services Vector of service implementations to benchmark
     * @param sampleCountry Representative country for country-specific tests
     * @param midYear Representative year for most operations
     * @param years Vector of available years for range operations
     * @param config Benchmark configuration
     */
    void runFullBenchmarkSuite(
        const std::vector<std::reference_wrapper<IPopulationService>>& services,
        const std::string& sampleCountry,
        int midYear,
        const std::vector<long long>& years,
        const BenchmarkConfig& config = {});

    /**
     * @brief Create service reference vector from concrete services
     * 
     * Utility function to create a vector of service references from
     * concrete service instances for use with the generic benchmark functions.
     * 
     * @param services Variable number of service references
     * @return Vector of service references suitable for benchmark functions
     */
    template<typename... Services>
    std::vector<std::reference_wrapper<IPopulationService>> createServiceVector(Services&... services) {
        return {std::reference_wrapper<IPopulationService>(services)...};
    }

} // namespace BenchmarkRunner
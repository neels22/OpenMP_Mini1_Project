#include "../interface/benchmark_runner.hpp"
#include "../interface/constants.hpp"
#include <iostream>
#include <algorithm>

namespace BenchmarkRunner {

    void runTopNBenchmark(
        const std::vector<std::reference_wrapper<IPopulationService>>& services,
        const std::string& operationName,
        int year,
        std::size_t n,
        const BenchmarkConfig& config) {
        
        for (const auto& serviceRef : services) {
            const IPopulationService& service = serviceRef.get();
            const std::string& implName = service.getImplementationName();
            
            std::vector<std::pair<std::string, long long>> serialResult, parallelResult;
            
            BenchmarkUtils::runAndReport(
                operationName + " (" + implName + ")",
                [&]{ serialResult = service.topNCountriesByPopulationInYear(year, n, 1); },
                [&]{ parallelResult = service.topNCountriesByPopulationInYear(year, n, config.parallelThreads); },
                config.repetitions
            );
            
            if (config.showValues) {
                std::cout << "  -> counts: serial_count=" << serialResult.size() 
                          << " parallel_count=" << parallelResult.size() << "\n";
            }
            
            if (config.validateResults && serialResult.size() != parallelResult.size()) {
                std::cout << "  ⚠️  WARNING: Serial/parallel result count mismatch!\n";
            }
        }
        std::cout << "\n";
    }

    void runYearRangeBenchmark(
        const std::vector<std::reference_wrapper<IPopulationService>>& services,
        const std::string& country,
        int startYear,
        int endYear,
        const BenchmarkConfig& config) {
        
        for (const auto& serviceRef : services) {
            const IPopulationService& service = serviceRef.get();
            const std::string& implName = service.getImplementationName();
            
            std::vector<long long> serialResult, parallelResult;
            
            BenchmarkUtils::runAndReport(
                "populationOverYearsForCountry (" + implName + ")",
                [&]{ serialResult = service.populationOverYearsForCountry(country, startYear, endYear, 1); },
                [&]{ parallelResult = service.populationOverYearsForCountry(country, startYear, endYear, config.parallelThreads); },
                config.repetitions
            );
            
            if (config.showValues) {
                std::cout << "  -> len=" << serialResult.size() << "\n";
            }
            
            if (config.validateResults && serialResult.size() != parallelResult.size()) {
                std::cout << "  ⚠️  WARNING: Serial/parallel result length mismatch!\n";
            }
        }
        std::cout << "\n";
    }

    void runFullBenchmarkSuite(
        const std::vector<std::reference_wrapper<IPopulationService>>& services,
        const std::string& sampleCountry,
        int midYear,
        const std::vector<long long>& years,
        const BenchmarkConfig& config) {
        
        std::cout << "========================================\n";
        std::cout << "   COMPREHENSIVE BENCHMARK SUITE\n";
        std::cout << "========================================\n\n";
        
        // === Aggregation Benchmarks ===
        std::cout << "=== Aggregation Operations ===\n\n";
        
        runAggregationBenchmark<long long>(
            services, "sumPopulationForYear",
            [midYear](const IPopulationService& svc, int numThreads) {
                return svc.sumPopulationForYear(midYear, numThreads);
            },
            midYear, config
        );
        
        runAggregationBenchmark<double>(
            services, "averagePopulationForYear",
            [midYear](const IPopulationService& svc, int numThreads) {
                return svc.averagePopulationForYear(midYear, numThreads);
            },
            midYear, config
        );
        
        runAggregationBenchmark<long long>(
            services, "maxPopulationForYear",
            [midYear](const IPopulationService& svc, int numThreads) {
                return svc.maxPopulationForYear(midYear, numThreads);
            },
            midYear, config
        );
        
        runAggregationBenchmark<long long>(
            services, "minPopulationForYear",
            [midYear](const IPopulationService& svc, int numThreads) {
                return svc.minPopulationForYear(midYear, numThreads);
            },
            midYear, config
        );
        
        // === Top-N Benchmarks ===
        std::cout << "=== Top-N Operations ===\n\n";
        
        runTopNBenchmark(services, "topNCountriesByPopulationInYear", midYear, Config::TOP_N_DEFAULT, config);
        
        // === Country-Specific Benchmarks ===
        std::cout << "=== Country-Specific Operations ===\n\n";
        
        runCountryBenchmark<long long>(
            services, "populationForCountryInYear",
            [midYear](const IPopulationService& svc, const std::string& country, int numThreads) {
                return svc.populationForCountryInYear(country, midYear, numThreads);
            },
            sampleCountry, config
        );
        
        // Year range benchmark
        if (!years.empty() && years.size() >= 2) {
            int startYear = static_cast<int>(years[0]);
            int endYear = static_cast<int>(years[std::min(years.size() - 1, static_cast<std::size_t>(10))]);
            
            runYearRangeBenchmark(services, sampleCountry, startYear, endYear, config);
        }
        
        std::cout << "========================================\n";
        std::cout << "   BENCHMARK SUITE COMPLETE\n";
        std::cout << "========================================\n\n";
    }

} // namespace BenchmarkRunner
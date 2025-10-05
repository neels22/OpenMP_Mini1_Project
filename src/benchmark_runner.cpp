#include "../interface/benchmark_runner.hpp"
#include "../interface/constants.hpp"
#include <iostream>
#include <algorithm>

namespace BenchmarkRunner {
    template<typename T>
    void runAggregationBenchmark(
        const std::vector<std::reference_wrapper<IPopulationService>>& services,
        const std::string& operationName,
        std::function<T(const IPopulationService&, int)> operation,
        int /* year */,
        const BenchmarkConfig& config) {
        for (const auto& serviceRef : services) {
            const IPopulationService& service = serviceRef.get();
            const std::string& implName = service.getImplementationName();
            T serialResult{}, parallelResult{};
            BenchmarkUtils::runAndReport(
                operationName + " (" + implName + ")",
                [&]{ serialResult = operation(service, 1); },
                [&]{ parallelResult = operation(service, config.parallelThreads); },
                config.repetitions
            );
            if (config.showValues) {
                std::cout << "  -> values: serial=" << serialResult 
                          << " parallel=" << parallelResult << "\n";
            }
            if (config.validateResults && serialResult != parallelResult) {
                std::cout << "  ⚠️  WARNING: Serial/parallel result mismatch!\n";
            }
        }
        std::cout << "\n";
    }

    template<typename T>
    void runCountryBenchmark(
        const std::vector<std::reference_wrapper<IPopulationService>>& services,
        const std::string& operationName,
        std::function<T(const IPopulationService&, const std::string&, int)> operation,
        const std::string& country,
        const BenchmarkConfig& config) {
        for (const auto& serviceRef : services) {
            const IPopulationService& service = serviceRef.get();
            const std::string& implName = service.getImplementationName();
            T serialResult{}, parallelResult{};
            BenchmarkUtils::runAndReport(
                operationName + " (" + implName + ")",
                [&]{ serialResult = operation(service, country, 1); },
                [&]{ parallelResult = operation(service, country, config.parallelThreads); },
                config.repetitions
            );
            if (config.showValues) {
                if constexpr (std::is_same_v<T, std::vector<long long>>) {
                    std::cout << "  -> len=" << serialResult.size() << "\n";
                } else {
                    std::cout << "  -> values: serial=" << serialResult 
                              << " parallel=" << parallelResult << "\n";
                }
            }
        }
        std::cout << "\n";
    }

    // Explicit instantiations for common types
    template void runAggregationBenchmark<long long>(
        const std::vector<std::reference_wrapper<IPopulationService>>&, const std::string&,
        std::function<long long(const IPopulationService&, int)>, int, const BenchmarkConfig&);
    template void runCountryBenchmark<long long>(
        const std::vector<std::reference_wrapper<IPopulationService>>&, const std::string&,
        std::function<long long(const IPopulationService&, const std::string&, int)>,
        const std::string&, const BenchmarkConfig&);
    template void runCountryBenchmark<std::vector<long long>>(
        const std::vector<std::reference_wrapper<IPopulationService>>&, const std::string&,
        std::function<std::vector<long long>(const IPopulationService&, const std::string&, int)>,
        const std::string&, const BenchmarkConfig&);

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
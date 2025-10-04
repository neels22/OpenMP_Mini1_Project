/**
 * @file main.cpp
 * @brief Main benchmark application for comparing row vs column population data models
 * 
 * This application provides comprehensive benchmarking of row-oriented vs column-oriented
 * population data models using identical service APIs. It demonstrates the performance
 * characteristics of different data layouts for various types of analytical operations.
 * 
 * Key Features:
 * - Supports both real and synthetic datasets
 * - Configurable repetitions and thread counts for statistical analysis
 * - Comprehensive error handling and validation
 * - Identical API testing for fair performance comparison
 * - OpenMP parallelization with scalable thread configuration
 * 
 * Usage Examples:
 * - Default execution: ./app
 * - Custom repetitions: ./app -r 10
 * - Custom threading: ./app -t 4
 * - Help information: ./app --help
 * - Custom CSV: CSV_PATH=data.csv ./app
 */

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <cstdlib>
#include "../interface/populationModel.hpp"
#include "../interface/service.hpp"
#include "../interface/readcsv.hpp"
#include "../interface/constants.hpp"
#include "../interface/benchmark_utils.hpp"
#include "../interface/utils.hpp"

namespace {
    /**
     * @brief Get CSV path from environment variable or use default
     * @return Path to CSV file for data loading
     * 
     * Checks CSV_PATH environment variable first, falls back to default
     * location if not set. This allows flexible data source configuration
     * without recompilation.
     */
    std::string getCSVPath() {
        const char* envCsv = std::getenv("CSV_PATH");
        return envCsv ? std::string(envCsv) : std::string("data/PopulationData/population.csv");
    }

    /**
     * @brief Print model information for verification and debugging
     * @param model Row-oriented model
     * @param modelCol Column-oriented model
     * 
     * Displays basic statistics about loaded models to verify
     * successful data loading and model consistency.
     */
    void printModelInfo(const PopulationModel& model, const PopulationModelColumn& modelCol) {
        std::cout << "Rows: " << model.rowCount() << " Years: " << model.years().size() << "\n";
        std::cout << "Rows (columnar): " << modelCol.rowCount() 
                  << " Years (columnar): " << modelCol.years().size() << "\n";
    }

    /**
     * @brief Run comprehensive aggregation benchmarks comparing row vs column models
     * @param svc Row-oriented service instance
     * @param svcCol Column-oriented service instance
     * @param midYear Representative year for benchmarking
     * @param parallelThreads Number of threads for parallel execution
     * @param repetitions Number of repetitions for statistical accuracy
     * 
     * Executes and compares performance of fundamental aggregation operations:
     * - Sum: Total population across all countries
     * - Average: Mean population across all countries
     * - Max: Highest population among all countries
     * - Min: Lowest population among all countries
     * 
     * Each operation is tested in both serial and parallel modes to
     * demonstrate scaling characteristics and cache locality effects.
     */
    void runAggregationBenchmarks(PopulationModelService& svc, 
                                 PopulationModelColumnService& svcCol,
                                 int midYear, int parallelThreads, int repetitions) {
        // Sum population benchmarks - demonstrates basic reduction operations
        {
            long long serialResult = 0, parallelResult = 0;
            BenchmarkUtils::runAndReport("sumPopulationForYear (row)",
                [&]{ serialResult = svc.sumPopulationForYear(midYear, 1); },
                [&]{ parallelResult = svc.sumPopulationForYear(midYear, parallelThreads); },
                repetitions);
            std::cout << "  -> values: serial=" << serialResult 
                      << " parallel=" << parallelResult << "\n";
        }
        
        {
            long long serialResult = 0, parallelResult = 0;
            BenchmarkUtils::runAndReport("sumPopulationForYear (col)",
                [&]{ serialResult = svcCol.sumPopulationForYear(midYear, 1); },
                [&]{ parallelResult = svcCol.sumPopulationForYear(midYear, parallelThreads); },
                repetitions);
            std::cout << "  -> values: serial=" << serialResult 
                      << " parallel=" << parallelResult << "\n";
        }

        // Average population benchmarks
        {
            double serialResult = 0, parallelResult = 0;
            BenchmarkUtils::runAndReport("averagePopulationForYear (row)",
                [&]{ serialResult = svc.averagePopulationForYear(midYear, 1); },
                [&]{ parallelResult = svc.averagePopulationForYear(midYear, parallelThreads); },
                repetitions);
            std::cout << "  -> values: serial=" << serialResult 
                      << " parallel=" << parallelResult << "\n";
        }
        
        {
            double serialResult = 0, parallelResult = 0;
            BenchmarkUtils::runAndReport("averagePopulationForYear (col)",
                [&]{ serialResult = svcCol.averagePopulationForYear(midYear, 1); },
                [&]{ parallelResult = svcCol.averagePopulationForYear(midYear, parallelThreads); },
                repetitions);
            std::cout << "  -> values: serial=" << serialResult 
                      << " parallel=" << parallelResult << "\n";
        }

        // Max population benchmarks
        {
            long long serialResult = 0, parallelResult = 0;
            BenchmarkUtils::runAndReport("maxPopulationForYear (row)",
                [&]{ serialResult = svc.maxPopulationForYear(midYear, 1); },
                [&]{ parallelResult = svc.maxPopulationForYear(midYear, parallelThreads); },
                repetitions);
            std::cout << "  -> values: serial=" << serialResult 
                      << " parallel=" << parallelResult << "\n";
        }
        
        {
            long long serialResult = 0, parallelResult = 0;
            BenchmarkUtils::runAndReport("maxPopulationForYear (col)",
                [&]{ serialResult = svcCol.maxPopulationForYear(midYear, 1); },
                [&]{ parallelResult = svcCol.maxPopulationForYear(midYear, parallelThreads); },
                repetitions);
            std::cout << "  -> values: serial=" << serialResult 
                      << " parallel=" << parallelResult << "\n";
        }

        // Min population benchmarks
        {
            long long serialResult = 0, parallelResult = 0;
            BenchmarkUtils::runAndReport("minPopulationForYear (row)",
                [&]{ serialResult = svc.minPopulationForYear(midYear, 1); },
                [&]{ parallelResult = svc.minPopulationForYear(midYear, parallelThreads); },
                repetitions);
            std::cout << "  -> values: serial=" << serialResult 
                      << " parallel=" << parallelResult << "\n";
        }
        
        {
            long long serialResult = 0, parallelResult = 0;
            BenchmarkUtils::runAndReport("minPopulationForYear (col)",
                [&]{ serialResult = svcCol.minPopulationForYear(midYear, 1); },
                [&]{ parallelResult = svcCol.minPopulationForYear(midYear, parallelThreads); },
                repetitions);
            std::cout << "  -> values: serial=" << serialResult 
                      << " parallel=" << parallelResult << "\n";
        }
    }

    /**
     * Run top-N benchmarks
     */
    void runTopNBenchmarks(PopulationModelService& svc, 
                          PopulationModelColumnService& svcCol,
                          int midYear, int parallelThreads, int repetitions) {
        constexpr int TOP_N = 10;
        
        {
            std::vector<std::pair<std::string,long long>> serialResult, parallelResult;
            BenchmarkUtils::runAndReport("topNCountriesByPopulationInYear (row)",
                [&]{ serialResult = svc.topNCountriesByPopulationInYear(midYear, TOP_N, 1); },
                [&]{ parallelResult = svc.topNCountriesByPopulationInYear(midYear, TOP_N, parallelThreads); },
                repetitions);
            std::cout << "  -> counts: serial_count=" << serialResult.size() 
                      << " parallel_count=" << parallelResult.size() << "\n";
        }
        
        {
            std::vector<std::pair<std::string,long long>> serialResult, parallelResult;
            BenchmarkUtils::runAndReport("topNCountriesByPopulationInYear (col)",
                [&]{ serialResult = svcCol.topNCountriesByPopulationInYear(midYear, TOP_N, 1); },
                [&]{ parallelResult = svcCol.topNCountriesByPopulationInYear(midYear, TOP_N, parallelThreads); },
                repetitions);
            std::cout << "  -> counts: serial_count=" << serialResult.size() 
                      << " parallel_count=" << parallelResult.size() << "\n";
        }
    }

    /**
     * Run country-specific benchmarks
     */
    void runCountryBenchmarks(PopulationModelService& svc, 
                             PopulationModelColumnService& svcCol,
                             const std::string& sampleCountry,
                             const std::vector<long long>& years,
                             int midYear, int parallelThreads, int repetitions) {
        if (sampleCountry.empty()) {
            std::cout << "No sample country available for country-specific benchmarks\n";
            return;
        }

        // Single year population
        {
            long long serialResult = 0, parallelResult = 0;
            BenchmarkUtils::runAndReport("populationForCountryInYear (row)",
                [&]{ serialResult = svc.populationForCountryInYear(sampleCountry, midYear, 1); },
                [&]{ parallelResult = svc.populationForCountryInYear(sampleCountry, midYear, parallelThreads); },
                repetitions);
            std::cout << "  -> values: serial=" << serialResult 
                      << " parallel=" << parallelResult << "\n";

            BenchmarkUtils::runAndReport("populationForCountryInYear (col)",
                [&]{ serialResult = svcCol.populationForCountryInYear(sampleCountry, midYear, 1); },
                [&]{ parallelResult = svcCol.populationForCountryInYear(sampleCountry, midYear, parallelThreads); },
                repetitions);
            std::cout << "  -> values: serial=" << serialResult 
                      << " parallel=" << parallelResult << "\n";
        }

        // Population over years
        {
            std::vector<long long> serialResult;
            int startYear = static_cast<int>(years.front());
            int endYear = static_cast<int>(years.back());
            
            BenchmarkUtils::runAndReport("populationOverYearsForCountry (row)",
                [&]{ serialResult = svc.populationOverYearsForCountry(sampleCountry, startYear, endYear, 1); },
                [&]{ (void)svc.populationOverYearsForCountry(sampleCountry, startYear, endYear, parallelThreads); },
                repetitions);
            std::cout << "  -> len=" << serialResult.size() << "\n";

            BenchmarkUtils::runAndReport("populationOverYearsForCountry (col)",
                [&]{ serialResult = svcCol.populationOverYearsForCountry(sampleCountry, startYear, endYear, 1); },
                [&]{ (void)svcCol.populationOverYearsForCountry(sampleCountry, startYear, endYear, parallelThreads); },
                repetitions);
            std::cout << "  -> len=" << serialResult.size() << "\n";
        }
    }
}

int main(int argc, char** argv) {
    // Parse command line arguments
    auto config = BenchmarkUtils::parseCommandLine(argc, argv);
    
    if (config.showHelp) {
        BenchmarkUtils::printUsage(argv[0]);
        return 0;
    }

    // Initialize models with error handling
    std::string csvPath = getCSVPath();
    PopulationModel model;
    PopulationModelColumn modelCol;
    
    auto initResult = BenchmarkUtils::initializeModels(csvPath, model, modelCol);
    if (!initResult.success) {
        std::cerr << "Error: " << initResult.errorMessage << "\n";
        return 1;
    }

    // Create services
    PopulationModelService svc(&model);
    PopulationModelColumnService svcCol(&modelCol);

    // Print model information
    printModelInfo(model, modelCol);

    // Get benchmark parameters
    int midYear = BenchmarkUtils::getSafeMidYear(model);
    std::string sampleCountry = BenchmarkUtils::getSafeSampleCountry(model);

    // Set output precision for timing results
    std::cout << std::fixed << std::setprecision(3);

    // Run all benchmark categories
    runAggregationBenchmarks(svc, svcCol, midYear, config.parallelThreads, config.repetitions);
    runTopNBenchmarks(svc, svcCol, midYear, config.parallelThreads, config.repetitions);
    runCountryBenchmarks(svc, svcCol, sampleCountry, model.years(), midYear, 
                        config.parallelThreads, config.repetitions);

    return 0;
}

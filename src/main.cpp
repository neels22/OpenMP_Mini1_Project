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
     * Get CSV path from environment or use default
     */
    std::string getCSVPath() {
        const char* envCsv = std::getenv("CSV_PATH");
        return envCsv ? std::string(envCsv) : std::string("data/PopulationData/population.csv");
    }

    /**
     * Print model information for verification
     */
    void printModelInfo(const PopulationModel& model, const PopulationModelColumn& modelCol) {
        std::cout << "Rows: " << model.rowCount() << " Years: " << model.years().size() << "\n";
        std::cout << "Rows (columnar): " << modelCol.rowCount() 
                  << " Years (columnar): " << modelCol.years().size() << "\n";
    }

    /**
     * Run aggregation benchmarks (sum, average, min, max)
     */
    void runAggregationBenchmarks(PopulationModelService& svc, 
                                 PopulationModelColumnService& svcCol,
                                 int midYear, int parallelThreads, int repetitions) {
        // Sum population benchmarks
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

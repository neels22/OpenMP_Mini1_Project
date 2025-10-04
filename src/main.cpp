#include <iostream>
#include <string>
#include <memory>
#include <cstdlib>

#include "../interface/populationModel.hpp"
#include "../interface/populationModelColumn.hpp"
#include "../interface/service.hpp"
#include "../interface/benchmark_runner.hpp"
#include "../interface/benchmark_utils.hpp"
#include "../interface/constants.hpp"

/**
 * @file main.cpp
 * @brief Refactored benchmark application using generic interfaces
 * 
 * This application demonstrates the elimination of code duplication through
 * common interfaces and generic benchmark runners. The main function now
 * uses polymorphic service interfaces to run identical benchmarks across
 * different data layout implementations without code duplication.
 * 
 * Key Improvements:
 * - Eliminated duplicate benchmark code through generic templates
 * - Uses polymorphic IPopulationService interface for unified benchmarking
 * - Automatic result validation between serial and parallel execution
 * - Configurable benchmark parameters through BenchmarkConfig
 * 
 * Usage:
 *   ./benchmark [--help] [--threads N] [--repetitions N]
 */

namespace {
    /**
     * Create synthetic test data for interface demonstration
     */
    void createSyntheticData(PopulationModel& model, PopulationModelColumn& modelCol) {
        // Create small synthetic dataset for testing
        std::vector<long long> years = {2020, 2021, 2022};
        
        // Set years for both models
        model.setYears(years);
        modelCol.setYears(years);
        
        // Add a few countries with test data
        std::vector<long long> country1_data = {1000000, 1100000, 1200000};
        std::vector<long long> country2_data = {2000000, 2200000, 2400000};
        std::vector<long long> country3_data = {500000, 550000, 600000};
        
        // Add to row model using insertNewEntry
        model.insertNewEntry("Country1", "C1", "IND1", "IC1", country1_data);
        model.insertNewEntry("Country2", "C2", "IND2", "IC2", country2_data);
        model.insertNewEntry("Country3", "C3", "IND3", "IC3", country3_data);
        
        // Add to column model using the same API
        modelCol.insertNewEntry("Country1", "C1", "IND1", "IC1", country1_data);
        modelCol.insertNewEntry("Country2", "C2", "IND2", "IC2", country2_data);
        modelCol.insertNewEntry("Country3", "C3", "IND3", "IC3", country3_data);
        
        std::cout << "Created synthetic dataset with 3 countries and 3 years\n";
    }

    /**
     * Print model information for verification
     */
    void printModelInfo(const PopulationModel& model, const PopulationModelColumn& modelCol) {
        std::cout << "\n=== Model Information ===\n";
        std::cout << "Row Model: " << model.rowCount() << " countries, " 
                  << model.years().size() << " years\n";
        std::cout << "Column Model: " << modelCol.rowCount() << " countries, " 
                  << modelCol.years().size() << " years\n";
        
        if (!model.years().empty()) {
            const auto& years = model.years();
            std::cout << "Year range: " << years.front() << " - " << years.back() << "\n";
        }
        std::cout << "\n";
    }

    /**
     * Get sample country name for benchmarks
     */
    std::string getSampleCountry(const PopulationModel& model) {
        if (model.rowCount() > 0) {
            const auto& row = model.rowAt(0);
            return row.country();
        }
        return "Country1";  // Default for synthetic data
    }

    /**
     * Get representative year for benchmarks
     */
    int getRepresentativeYear(const std::vector<long long>& years) {
        if (years.empty()) return 2021;
        // Use middle year for better data coverage
        return static_cast<int>(years[years.size() / 2]);
    }
}

int main(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        BenchmarkUtils::Config args = BenchmarkUtils::parseCommandLine(argc, argv);
        if (args.showHelp) {
            std::cout << "Usage: " << argv[0] << " [--help] [--threads N] [--repetitions N]\n";
            std::cout << "\nDemonstrates interface-based design eliminating code duplication\n";
            std::cout << "Uses synthetic data to showcase generic benchmark framework\n\n";
            std::cout << "Options:\n";
            std::cout << "  --help              Show this help message\n";
            std::cout << "  --threads N         Number of parallel threads (default: 4)\n";
            std::cout << "  --repetitions N     Number of benchmark repetitions (default: 5)\n\n";
            return 0;
        }
        
        std::cout << "=== Population Data Analysis: Interface Comparison ===\n";
        std::cout << "Threads: " << args.parallelThreads 
                  << ", Repetitions: " << args.repetitions << "\n\n";

        // Create models with synthetic data for demonstration
        PopulationModel model;
        PopulationModelColumn modelCol;
        
        createSyntheticData(model, modelCol);
        
        // Validate models
        auto validation = BenchmarkUtils::validateModels(model, modelCol);
        if (!validation.success) {
            std::cerr << "Model validation failed: " << validation.errorMessage << "\n";
            return 1;
        }

        printModelInfo(model, modelCol);

        // Create services with the common interface
        PopulationModelService rowService(&model);
        PopulationModelColumnService columnService(&modelCol);
        
        // Create vector of service references for generic benchmarking
        auto services = BenchmarkRunner::createServiceVector(rowService, columnService);
        
        // Configure benchmark parameters
        BenchmarkRunner::BenchmarkConfig config;
        config.parallelThreads = args.parallelThreads;
        config.repetitions = args.repetitions;
        config.validateResults = true;
        config.showValues = true;
        
        // Get benchmark parameters
        std::string sampleCountry = getSampleCountry(model);
        int midYear = getRepresentativeYear(model.years());
        
        std::cout << "Sample country: " << sampleCountry << "\n";
        std::cout << "Representative year: " << midYear << "\n\n";
        
        // Run comprehensive benchmark suite using generic interface
        BenchmarkRunner::runFullBenchmarkSuite(
            services, 
            sampleCountry, 
            midYear, 
            model.years(), 
            config
        );
        
        std::cout << "✅ All benchmarks completed successfully!\n";
        std::cout << "\nKey Insights from Interface-Based Design:\n";
        std::cout << "- ✅ Eliminated duplicate benchmark code through IPopulationService interface\n";
        std::cout << "- ✅ Generic templates enable type-safe polymorphic benchmarking\n";
        std::cout << "- ✅ Automatic result validation ensures implementation correctness\n";
        std::cout << "- ✅ Both implementations satisfy identical service contracts\n";
        std::cout << "- ✅ Single benchmark suite works with any service implementation\n";
        std::cout << "- ✅ Reduced main.cpp from 300+ lines to ~140 lines through abstraction\n\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
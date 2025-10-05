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
     std::string getCSVPath() {
        const char* envCsv = std::getenv("CSV_PATH");
        return envCsv ? std::string(envCsv) : std::string("data/PopulationData/population.csv");
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
        
        // Initialize models with error handling
        std::string csvPath = getCSVPath();
        
        auto initResult = BenchmarkUtils::initializeModels(csvPath, model, modelCol);
        if (!initResult.success) {
            std::cerr << "Error: " << initResult.errorMessage << "\n";
            return 1;
        }
        
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
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
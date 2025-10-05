#include <iostream>
#include <string>
#include <memory>
#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <algorithm>

#include "../interface/populationModel.hpp"
#include "../interface/populationModelColumn.hpp"
#include "../interface/service.hpp"
#include "../interface/benchmark_runner.hpp"
#include "../interface/benchmark_utils.hpp"
#include "../interface/constants.hpp"
#include "../interface/fireRowModel.hpp"
#include "../interface/fireColumnModel.hpp"

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
    std::cout << "Column Model: " << modelCol.columnCount() << " countries, " 
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

    /**
     * Get fire data directory path
     */
    std::string getFireDataPath() {
        const char* envFireData = std::getenv("FIRE_DATA_PATH");
        return envFireData ? std::string(envFireData) : std::string("data/FireData");
    }

    /**
     * Benchmark fire data reading performance (serial vs parallel)
     */
    void benchmarkFireDataReading(const std::string& fireDataPath, int maxThreads, int repetitions) {
        std::cout << "\n=== Fire Data Reading Performance Benchmark ===\n";
        std::cout << "Fire data path: " << fireDataPath << "\n";
        std::cout << "Max threads: " << maxThreads << ", Repetitions: " << repetitions << "\n\n";

        // Get list of CSV files for testing
        std::vector<std::string> csv_files;
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(fireDataPath)) {
                if (entry.is_regular_file() && entry.path().extension() == ".csv") {
                    csv_files.push_back(entry.path().string());
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error scanning fire data directory: " << e.what() << "\n";
            return;
        }

        if (csv_files.empty()) {
            std::cout << "No CSV files found in " << fireDataPath << "\n";
            return;
        }

        // Sort for consistent ordering
        std::sort(csv_files.begin(), csv_files.end());
        std::cout << "Found " << csv_files.size() << " CSV files to process.\n\n";

        // Test different thread counts
        std::vector<int> thread_counts = {1, 2, 3, 4, maxThreads};
        // Remove duplicates and ensure we don't exceed available files
        std::sort(thread_counts.begin(), thread_counts.end());
        thread_counts.erase(std::unique(thread_counts.begin(), thread_counts.end()), thread_counts.end());
        thread_counts.erase(std::remove_if(thread_counts.begin(), thread_counts.end(), 
                                         [&](int t) { return t > std::min(maxThreads, static_cast<int>(csv_files.size())); }), 
                           thread_counts.end());

        std::cout << std::setw(15) << "Model" 
                  << std::setw(10) << "Threads" 
                  << std::setw(15) << "Avg Time (s)" 
                  << std::setw(12) << "Speedup" 
                  << std::setw(15) << "Sites" 
                  << std::setw(18) << "Measurements" 
                  << std::setw(12) << "Files/sec" 
                  << "\n";
        std::cout << std::string(100, '-') << "\n";

        double row_baseline_time = 0.0;
        double column_baseline_time = 0.0;
        
        for (int num_threads : thread_counts) {
            // Benchmark FireRowModel
            {
                std::vector<double> run_times;
                std::size_t final_sites = 0;
                std::size_t final_measurements = 0;

                for (int rep = 0; rep < repetitions; ++rep) {
                    FireRowModel fire_model;
                    
                    auto start = std::chrono::high_resolution_clock::now();
                    
                    try {
                        if (num_threads == 1) {
                            fire_model.readFromMultipleCSV(csv_files);
                        } else {
                            fire_model.readFromMultipleCSVParallel(csv_files, num_threads);
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Error processing files with FireRowModel " << num_threads << " threads: " << e.what() << "\n";
                        continue;
                    }
                    
                    auto end = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration<double>(end - start);
                    run_times.push_back(duration.count());
                    
                    if (rep == 0) {  // Store results from first run
                        final_sites = fire_model.siteCount();
                        final_measurements = fire_model.totalMeasurements();
                    }
                }

                if (!run_times.empty()) {
                    // Calculate statistics
                    double avg_time = 0.0;
                    for (double time : run_times) avg_time += time;
                    avg_time /= run_times.size();

                    if (num_threads == 1) {
                        row_baseline_time = avg_time;
                    }

                    double speedup = (row_baseline_time > 0) ? row_baseline_time / avg_time : 1.0;
                    double files_per_sec = csv_files.size() / avg_time;

                    std::cout << std::setw(15) << "Row-oriented" 
                              << std::setw(10) << num_threads 
                              << std::setw(15) << std::fixed << std::setprecision(3) << avg_time
                              << std::setw(12) << std::fixed << std::setprecision(2) << speedup << "x"
                              << std::setw(15) << final_sites
                              << std::setw(18) << final_measurements
                              << std::setw(12) << std::fixed << std::setprecision(1) << files_per_sec
                              << "\n";
                }
            }

            // Benchmark FireColumnModel
            {
                std::vector<double> run_times;
                std::size_t final_sites = 0;
                std::size_t final_measurements = 0;

                for (int rep = 0; rep < repetitions; ++rep) {
                    FireColumnModel fire_model;
                    
                    auto start = std::chrono::high_resolution_clock::now();
                    
                    try {
                        fire_model.readFromDirectory(fireDataPath, num_threads);
                    } catch (const std::exception& e) {
                        std::cerr << "Error processing files with FireColumnModel " << num_threads << " threads: " << e.what() << "\n";
                        continue;
                    }
                    
                    auto end = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration<double>(end - start);
                    run_times.push_back(duration.count());
                    
                    if (rep == 0) {  // Store results from first run
                        final_sites = fire_model.siteCount();
                        final_measurements = fire_model.measurementCount();
                    }
                }

                if (!run_times.empty()) {
                    // Calculate statistics
                    double avg_time = 0.0;
                    for (double time : run_times) avg_time += time;
                    avg_time /= run_times.size();

                    if (num_threads == 1) {
                        column_baseline_time = avg_time;
                    }

                    double speedup = (column_baseline_time > 0) ? column_baseline_time / avg_time : 1.0;
                    double files_per_sec = csv_files.size() / avg_time;

                    std::cout << std::setw(15) << "Column-oriented" 
                              << std::setw(10) << num_threads 
                              << std::setw(15) << std::fixed << std::setprecision(3) << avg_time
                              << std::setw(12) << std::fixed << std::setprecision(2) << speedup << "x"
                              << std::setw(15) << final_sites
                              << std::setw(18) << final_measurements
                              << std::setw(12) << std::fixed << std::setprecision(1) << files_per_sec
                              << "\n";
                }
            }

            if (num_threads < thread_counts.back()) {
                std::cout << std::string(100, '-') << "\n";
            }
        }
        
        std::cout << std::string(100, '-') << "\n\n";
        
        // Explain the benchmark metrics
        std::cout << "=== Benchmark Metrics Explained ===\n";
        std::cout << "Model: Data storage architecture (Row-oriented stores by sites, Column-oriented stores by fields)\n";
        std::cout << "Threads: Number of parallel OpenMP threads used for CSV processing\n";
        std::cout << "Avg Time: Average processing time in seconds (lower is better)\n";
        std::cout << "Speedup: Performance improvement vs single-threaded baseline (higher is better)\n";
        std::cout << "Sites: Number of unique monitoring sites found in the data\n";
        std::cout << "Measurements: Total number of fire/air quality measurements processed\n";
        std::cout << "Files/sec: Processing throughput - CSV files processed per second\n\n";

        // Summary comparison
        if (row_baseline_time > 0.0 && column_baseline_time > 0.0) {
            std::cout << "\n=== Model Comparison Summary ===\n";
            std::cout << "Serial Performance Comparison:\n";
            if (row_baseline_time < column_baseline_time) {
                double improvement = column_baseline_time / row_baseline_time;
                std::cout << "Row-oriented model is " << std::fixed << std::setprecision(2) 
                          << improvement << "x faster than Column-oriented for CSV ingestion\n";
            } else {
                double improvement = row_baseline_time / column_baseline_time;
                std::cout << "Column-oriented model is " << std::fixed << std::setprecision(2) 
                          << improvement << "x faster than Row-oriented for CSV ingestion\n";
            }
            std::cout << "Row-oriented baseline: " << std::fixed << std::setprecision(3) << row_baseline_time << "s\n";
            std::cout << "Column-oriented baseline: " << std::fixed << std::setprecision(3) << column_baseline_time << "s\n";
        }

        std::cout << "\nBenchmark completed successfully.\n";
    }

}

int main(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        BenchmarkUtils::Config args = BenchmarkUtils::parseCommandLine(argc, argv);
        
        // Check for fire benchmarking flag
        bool runFireBenchmark = false;
        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "--fire" || std::string(argv[i]) == "-f") {
                runFireBenchmark = true;
                break;
            }
        }
        
        if (args.showHelp) {
            std::cout << "Usage: " << argv[0] << " [--help] [--threads N] [--repetitions N] [--fire]\n";
            std::cout << "\nDemonstrates interface-based design eliminating code duplication\n";
            std::cout << "Uses synthetic data to showcase generic benchmark framework\n\n";
            std::cout << "Options:\n";
            std::cout << "  --help              Show this help message\n";
            std::cout << "  --threads N         Number of parallel threads (default: 4)\n";
            std::cout << "  --repetitions N     Number of benchmark repetitions (default: 5)\n";
            std::cout << "  --fire, -f          Run fire data reading benchmark\n\n";
            return 0;
        }
        
        std::cout << "=== Population Data Analysis: Interface Comparison ===\n";
        std::cout << "Threads: " << args.parallelThreads 
                  << ", Repetitions: " << args.repetitions << "\n\n";

        // Run fire data benchmark if requested
        if (runFireBenchmark) {
            std::string fireDataPath = getFireDataPath();
            benchmarkFireDataReading(fireDataPath, args.parallelThreads, args.repetitions);
            std::cout << "\n" << std::string(60, '=') << "\n";
        }

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
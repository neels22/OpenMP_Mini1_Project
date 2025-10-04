#pragma once

#include <string>
#include <functional>
#include "../interface/service.hpp"
#include "../interface/populationModel.hpp"

namespace BenchmarkUtils {
    /**
     * Configuration structure for command line arguments
     */
    struct Config {
        int repetitions;
        int parallelThreads;
        bool showHelp;
        
        Config();
    };

    /**
     * Result structure for validation
     */
    struct ValidationResult {
        bool success;
        std::string errorMessage;
        
        ValidationResult(bool success = true, const std::string& error = "");
    };

    /**
     * Parse command line arguments and return configuration
     */
    Config parseCommandLine(int argc, char** argv);
    
    /**
     * Display usage information
     */
    void printUsage(const char* programName);
    
    /**
     * Validate models and services before running benchmarks
     */
    ValidationResult validateModels(const PopulationModel& model, 
                                   const PopulationModelColumn& modelCol);
    
    /**
     * Initialize models from CSV with error handling
     */
    ValidationResult initializeModels(const std::string& csvPath,
                                     PopulationModel& model,
                                     PopulationModelColumn& modelCol);
    
    /**
     * Run and report a single benchmark comparison
     */
    void runAndReport(const std::string& label,
                     const std::function<void()>& serialFn,
                     const std::function<void()>& parallelFn,
                     int repetitions);
    
    /**
     * Get safe mid-year from model years
     */
    int getSafeMidYear(const PopulationModel& model);
    
    /**
     * Get safe sample country from model
     */
    std::string getSafeSampleCountry(const PopulationModel& model);
}
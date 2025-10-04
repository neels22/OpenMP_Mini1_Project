#pragma once

#include <string>
#include <functional>
#include "../interface/service.hpp"
#include "../interface/populationModel.hpp"

/**
 * @file benchmark_utils.hpp
 * @brief Comprehensive benchmark framework for population data analysis
 * 
 * This file provides utilities for command-line parsing, model validation,
 * error handling, and benchmark execution. The framework supports both
 * row and column data models with consistent interfaces and robust
 * error handling throughout.
 */

namespace BenchmarkUtils {
    /**
     * @struct Config
     * @brief Configuration structure for command line arguments and benchmark parameters
     * 
     * Encapsulates all configurable parameters for benchmark execution,
     * providing sensible defaults and type-safe parameter management.
     */
    struct Config {
        int repetitions;        ///< Number of repetitions per measurement for statistical accuracy
        int parallelThreads;    ///< Number of threads for parallel execution
        bool showHelp;          ///< Flag indicating user requested help information
        
        /// Constructor with intelligent defaults based on system capabilities
        Config();
    };

    /**
     * @struct ValidationResult
     * @brief Result structure for model and operation validation
     * 
     * Provides structured error reporting with success flags and descriptive
     * error messages for debugging and user feedback.
     */
    struct ValidationResult {
        bool success;           ///< True if validation passed, false otherwise
        std::string errorMessage; ///< Detailed error description for failures
        
        /// Constructor for creating validation results
        ValidationResult(bool success = true, const std::string& error = "");
    };

    // === Command Line Interface ===
    
    /**
     * @brief Parse command line arguments and return configuration
     * @param argc Argument count from main()
     * @param argv Argument vector from main()
     * @return Populated Config structure with parsed values
     * 
     * Supports multiple argument formats:
     * - Flags: -h, --help, -r, --reps, -t, --threads
     * - Key-value: --reps=5, --threads=4
     * - Positional: first number is repetitions, second is threads
     * 
     * Provides robust error handling with fallback to defaults for invalid inputs.
     */
    Config parseCommandLine(int argc, char** argv);
    
    /**
     * @brief Display usage information and command-line help
     * @param programName Name of the executable (argv[0])
     * 
     * Shows comprehensive help including all supported flags, examples,
     * and default values for user guidance.
     */
    void printUsage(const char* programName);
    
    // === Model Validation ===
    
    /**
     * @brief Validate models and services before running benchmarks
     * @param model Row-oriented population model to validate
     * @param modelCol Column-oriented population model to validate
     * @return ValidationResult indicating success or failure with error details
     * 
     * Performs comprehensive validation including:
     * - Data presence checks (non-empty years and countries)
     * - Model consistency verification (matching dimensions)
     * - Data integrity validation
     */
    ValidationResult validateModels(const PopulationModel& model, 
                                   const PopulationModelColumn& modelCol);
    
    /**
     * @brief Initialize models from CSV with comprehensive error handling
     * @param csvPath Path to CSV file to load
     * @param model Row-oriented model to populate
     * @param modelCol Column-oriented model to populate
     * @return ValidationResult indicating success or failure with error details
     * 
     * Handles all aspects of model initialization:
     * - File existence and accessibility
     * - CSV parsing and data loading
     * - Model validation after loading
     * - Exception handling with detailed error reporting
     */
    ValidationResult initializeModels(const std::string& csvPath,
                                     PopulationModel& model,
                                     PopulationModelColumn& modelCol);
    
    // === Benchmark Execution ===
    
    /**
     * @brief Run and report a single benchmark comparison between serial and parallel
     * @param label Descriptive name for the benchmark operation
     * @param serialFn Function to execute in serial mode
     * @param parallelFn Function to execute in parallel mode
     * @param repetitions Number of times to repeat each measurement
     * 
     * Executes both serial and parallel versions multiple times,
     * calculates median and standard deviation, and reports results
     * in a standardized format for comparison analysis.
     */
    void runAndReport(const std::string& label,
                     const std::function<void()>& serialFn,
                     const std::function<void()>& parallelFn,
                     int repetitions);
    
    // === Data Safety Utilities ===
    
    /**
     * @brief Get safe mid-year from model years with fallback
     * @param model Model to extract year information from
     * @return Middle year from model, or default year if model is empty
     * 
     * Provides safe access to a representative year for benchmarking,
     * with graceful handling of empty or invalid datasets.
     */
    int getSafeMidYear(const PopulationModel& model);
    
    /**
     * @brief Get safe sample country from model with fallback
     * @param model Model to extract country information from
     * @return First country name, or empty string if no countries available
     * 
     * Provides safe access to a representative country for benchmarking,
     * with graceful handling of empty datasets.
     */
    std::string getSafeSampleCountry(const PopulationModel& model);
}
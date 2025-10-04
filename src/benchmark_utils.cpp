#include "../interface/benchmark_utils.hpp"
#include "../interface/constants.hpp"
#include "../interface/utils.hpp"
#include <iostream>
#include <thread>
#include <iomanip>
#include <cstdlib>

namespace BenchmarkUtils {
    
    Config::Config() 
        : repetitions(::Config::DEFAULT_REPETITIONS)
        , parallelThreads(std::thread::hardware_concurrency() > 0 
                         ? static_cast<int>(std::thread::hardware_concurrency()) 
                         : ::Config::DEFAULT_THREADS_FALLBACK)
        , showHelp(false) {
    }

    ValidationResult::ValidationResult(bool success, const std::string& error)
        : success(success), errorMessage(error) {
    }

    Config parseCommandLine(int argc, char** argv) {
        Config config;
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "-h" || arg == "--help") {
                config.showHelp = true;
                return config;
            }
            
            if (arg == "-r" || arg == "--reps") {
                if (i + 1 < argc) {
                    try {
                        int reps = std::stoi(argv[++i]);
                        if (reps >= 1) {
                            config.repetitions = reps;
                        }
                    } catch (const std::exception&) {
                        // Keep default value on parse error
                    }
                }
                continue;
            }
            
            if (arg.rfind("--reps=", 0) == 0) {
                try {
                    int reps = std::stoi(arg.substr(7));
                    if (reps >= 1) {
                        config.repetitions = reps;
                    }
                } catch (const std::exception&) {
                    // Keep default value on parse error
                }
                continue;
            }
            
            if (arg == "-t" || arg == "--threads") {
                if (i + 1 < argc) {
                    try {
                        int threads = std::stoi(argv[++i]);
                        if (threads > 0) {
                            config.parallelThreads = threads;
                        }
                    } catch (const std::exception&) {
                        // Keep default value on parse error
                    }
                }
                continue;
            }
            
            if (arg.rfind("--threads=", 0) == 0) {
                try {
                    int threads = std::stoi(arg.substr(10));
                    if (threads > 0) {
                        config.parallelThreads = threads;
                    }
                } catch (const std::exception&) {
                    // Keep default value on parse error
                }
                continue;
            }
            
            // Backward-compatible positional arguments
            try {
                int value = std::stoi(arg);
                if (config.repetitions == ::Config::DEFAULT_REPETITIONS) {
                    config.repetitions = value > 0 ? value : ::Config::DEFAULT_REPETITIONS;
                    continue;
                }
                if (value > 0) {
                    config.parallelThreads = value;
                }
            } catch (const std::exception&) {
                // Ignore unknown arguments
            }
        }
        
        return config;
    }
    
    void printUsage(const char* programName) {
        std::cout << "Usage: " << programName << " [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  -h, --help           Show this help message and exit\n";
        std::cout << "  -r N, --reps N       Number of repetitions per measurement (default " 
                  << ::Config::DEFAULT_REPETITIONS << ")\n";
        std::cout << "  -t N, --threads N    Number of threads to use for parallel runs (default = hardware)\n";
        std::cout << "\nExamples:\n";
        std::cout << "  # run 5 repetitions and auto thread count\n";
        std::cout << "  " << programName << " -r 5\n";
        std::cout << "  # run 10 repetitions with 2 threads\n";
        std::cout << "  " << programName << " -r 10 -t 2\n";
    }
    
    ValidationResult validateModels(const PopulationModel& model, 
                                   const PopulationModelColumn& modelCol) {
        if (model.years().empty()) {
            return ValidationResult(false, "No year columns found in row model");
        }
        
        if (modelCol.years().empty()) {
            return ValidationResult(false, "No year columns found in column model");
        }
        
        if (model.rowCount() == 0) {
            return ValidationResult(false, "No data rows found in row model");
        }
        
        if (modelCol.rowCount() == 0) {
            return ValidationResult(false, "No data rows found in column model");
        }
        
        if (model.years().size() != modelCol.years().size()) {
            return ValidationResult(false, "Year count mismatch between models");
        }
        
        if (model.rowCount() != modelCol.rowCount()) {
            return ValidationResult(false, "Row count mismatch between models");
        }
        
        return ValidationResult(true);
    }
    
    ValidationResult initializeModels(const std::string& csvPath,
                                     PopulationModel& model,
                                     PopulationModelColumn& modelCol) {
        try {
            model.readFromCSV(csvPath);
        } catch (const std::exception& e) {
            return ValidationResult(false, "Failed to read CSV into row model: " + std::string(e.what()));
        } catch (...) {
            return ValidationResult(false, "Unknown error reading CSV into row model");
        }
        
        try {
            modelCol.readFromCSV(csvPath);
        } catch (const std::exception& e) {
            return ValidationResult(false, "Failed to read CSV into column model: " + std::string(e.what()));
        } catch (...) {
            return ValidationResult(false, "Unknown error reading CSV into column model");
        }
        
        return validateModels(model, modelCol);
    }
    
    void runAndReport(const std::string& label,
                     const std::function<void()>& serialFn,
                     const std::function<void()>& parallelFn,
                     int repetitions) {
        auto serialTimes = Utils::timeCallMulti(serialFn, repetitions);
        auto parallelTimes = Utils::timeCallMulti(parallelFn, repetitions);
        
        double serialMedian = Utils::median(serialTimes);
        double parallelMedian = Utils::median(parallelTimes);
        double serialStddev = Utils::stddev(serialTimes);
        double parallelStddev = Utils::stddev(parallelTimes);
        
        std::cout << std::fixed << std::setprecision(3);
        std::cout << label << ": serial_t_median=" << serialMedian 
                  << " us stddev=" << serialStddev 
                  << ", parallel_t_median=" << parallelMedian 
                  << " us stddev=" << parallelStddev << "\n";
    }
    
    int getSafeMidYear(const PopulationModel& model) {
        const auto& years = model.years();
        if (years.empty()) {
            return ::Config::DEFAULT_BASE_YEAR; // fallback year
        }
        return static_cast<int>(years[years.size() / 2]);
    }
    
    std::string getSafeSampleCountry(const PopulationModel& model) {
        const auto& countries = model.countryNames();
        if (countries.empty()) {
            return std::string(); // empty string indicates no sample available
        }
        return countries[0];
    }
}
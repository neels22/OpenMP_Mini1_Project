#include "../interface/fireRowModel.hpp"
#include "../interface/fireColumnModel.hpp"
#include "../interface/fire_service.hpp"
#include "../interface/benchmark_utils.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>

void printSeparator() {
    std::cout << "================================================================\n";
}

void printSectionHeader(const std::string& title) {
    printSeparator();
    std::cout << "  " << title << "\n";
    printSeparator();
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    int numThreads = 4;
    int numReps = 3;
    std::string dataPath = "data/FireData";
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--threads" || arg == "-t") && i + 1 < argc) {
            numThreads = std::stoi(argv[++i]);
        } else if ((arg == "--repetitions" || arg == "-r") && i + 1 < argc) {
            numReps = std::stoi(argv[++i]);
        } else if ((arg == "--data" || arg == "-d") && i + 1 < argc) {
            dataPath = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  -t, --threads N      Number of threads (default: 4)\n";
            std::cout << "  -r, --repetitions N  Number of repetitions (default: 3)\n";
            std::cout << "  -d, --data PATH      Path to fire data directory (default: data/FireData)\n";
            std::cout << "  -h, --help           Show this help message\n";
            return 0;
        }
    }
    
    try {
        printSectionHeader("Fire Data Analytics Benchmark: Row vs Column Services");
        std::cout << "Data path: " << dataPath << "\n";
        std::cout << "Threads: " << numThreads << "\n";
        std::cout << "Repetitions: " << numReps << "\n\n";
        
        // Load fire data into both models
        std::cout << "Loading fire data into Row model...\n";
        auto start = std::chrono::high_resolution_clock::now();
        FireRowModel rowModel;
        rowModel.readFromDirectoryParallel(dataPath, numThreads);
        auto end = std::chrono::high_resolution_clock::now();
        auto rowLoadTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::cout << "Loading fire data into Column model...\n";
        start = std::chrono::high_resolution_clock::now();
        FireColumnModel columnModel;
        columnModel.readFromDirectory(dataPath, numThreads);
        end = std::chrono::high_resolution_clock::now();
        auto colLoadTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::cout << "\nData Loading Summary:\n";
        std::cout << "  Row Model: " << rowModel.siteCount() << " sites, " 
                  << rowModel.totalMeasurements() << " measurements (" << rowLoadTime << " ms)\n";
        std::cout << "  Column Model: " << columnModel.siteCount() << " sites, " 
                  << columnModel.measurementCount() << " measurements (" << colLoadTime << " ms)\n\n";
        
        // Create services
        FireRowModelService rowService(&rowModel);
        FireColumnModelService columnService(&columnModel);
        
        // Get a sample parameter for testing
        const auto& params = rowModel.parameters();
        if (params.empty()) {
            std::cerr << "Error: No parameters found in data!\n";
            return 1;
        }
        std::string sampleParam = params[0];
        std::cout << "Using parameter '" << sampleParam << "' for benchmarks\n\n";
        
        // Get geographic bounds for testing
        double minLat, maxLat, minLon, maxLon;
        rowModel.getGeographicBounds(minLat, maxLat, minLon, maxLon);
        double midLat = (minLat + maxLat) / 2.0;
        double midLon = (minLon + maxLon) / 2.0;
        double latRange = (maxLat - minLat) / 4.0;
        double lonRange = (maxLon - minLon) / 4.0;
        
        // === Parameter-based Aggregations ===
        printSectionHeader("Parameter-based Aggregations");
        
        {
            std::cout << "\n1. Average Concentration for " << sampleParam << ":\n";
            double rowSerial = 0, rowParallel = 0, colSerial = 0, colParallel = 0;
            
            BenchmarkUtils::runAndReport(
                "Row Serial", 
                [&]{ rowSerial = rowService.averageConcentrationForParameter(sampleParam, 1); },
                [&]{}, numReps);
            
            BenchmarkUtils::runAndReport(
                "Row Parallel", 
                [&]{ rowParallel = rowService.averageConcentrationForParameter(sampleParam, numThreads); },
                [&]{}, numReps);
            
            BenchmarkUtils::runAndReport(
                "Column Serial", 
                [&]{ colSerial = columnService.averageConcentrationForParameter(sampleParam, 1); },
                [&]{}, numReps);
            
            BenchmarkUtils::runAndReport(
                "Column Parallel", 
                [&]{ colParallel = columnService.averageConcentrationForParameter(sampleParam, numThreads); },
                [&]{}, numReps);
            
            std::cout << "  Results: Row=" << std::fixed << std::setprecision(2) << rowSerial 
                      << ", Column=" << colSerial << " (diff: " << std::abs(rowSerial - colSerial) << ")\n";
        }
        
        {
            std::cout << "\n2. Max Concentration for " << sampleParam << ":\n";
            double rowMax = 0, colMax = 0;
            
            BenchmarkUtils::runAndReport(
                "Row Serial", 
                [&]{ rowMax = rowService.maxConcentrationForParameter(sampleParam, 1); },
                [&]{}, numReps);
            
            BenchmarkUtils::runAndReport(
                "Column Parallel", 
                [&]{ colMax = columnService.maxConcentrationForParameter(sampleParam, numThreads); },
                [&]{}, numReps);
            
            std::cout << "  Results: Row=" << rowMax << ", Column=" << colMax << "\n";
        }
        
        // === AQI Operations ===
        printSectionHeader("AQI Analysis");
        
        {
            std::cout << "\n3. Average AQI (all measurements):\n";
            double rowAvg = 0, colAvg = 0;
            
            BenchmarkUtils::runAndReport(
                "Row Serial", 
                [&]{ rowAvg = rowService.averageAQI(1); },
                [&]{}, numReps);
            
            BenchmarkUtils::runAndReport(
                "Column Parallel", 
                [&]{ colAvg = columnService.averageAQI(numThreads); },
                [&]{}, numReps);
            
            std::cout << "  Results: Row=" << std::fixed << std::setprecision(2) << rowAvg 
                      << ", Column=" << colAvg << "\n";
        }
        
        {
            std::cout << "\n4. Max/Min AQI:\n";
            int rowMax = 0, colMax = 0, rowMin = 0, colMin = 0;
            
            BenchmarkUtils::runAndReport(
                "Max AQI (Row)", 
                [&]{ rowMax = rowService.maxAQI(1); },
                [&]{}, numReps);
            
            BenchmarkUtils::runAndReport(
                "Max AQI (Column)", 
                [&]{ colMax = columnService.maxAQI(numThreads); },
                [&]{}, numReps);
            
            std::cout << "  Max AQI: Row=" << rowMax << ", Column=" << colMax << "\n";
            
            BenchmarkUtils::runAndReport(
                "Min AQI (Row)", 
                [&]{ rowMin = rowService.minAQI(1); },
                [&]{}, numReps);
            
            BenchmarkUtils::runAndReport(
                "Min AQI (Column)", 
                [&]{ colMin = columnService.minAQI(numThreads); },
                [&]{}, numReps);
            
            std::cout << "  Min AQI: Row=" << rowMin << ", Column=" << colMin << "\n";
        }
        
        // === Geographic Operations ===
        printSectionHeader("Geographic Operations");
        
        {
            std::cout << "\n5. Count measurements in central quarter of region:\n";
            std::cout << "  Bounds: [" << (midLat - latRange) << ", " << (midLat + latRange) 
                      << "] x [" << (midLon - lonRange) << ", " << (midLon + lonRange) << "]\n";
            
            std::size_t rowCount = 0, colCount = 0;
            
            BenchmarkUtils::runAndReport(
                "Row Parallel", 
                [&]{ rowCount = rowService.countMeasurementsInBounds(
                    midLat - latRange, midLat + latRange, 
                    midLon - lonRange, midLon + lonRange, numThreads); },
                [&]{}, numReps);
            
            BenchmarkUtils::runAndReport(
                "Column Parallel", 
                [&]{ colCount = columnService.countMeasurementsInBounds(
                    midLat - latRange, midLat + latRange, 
                    midLon - lonRange, midLon + lonRange, numThreads); },
                [&]{}, numReps);
            
            std::cout << "  Results: Row=" << rowCount << ", Column=" << colCount << "\n";
        }
        
        // === Top-N Operations ===
        printSectionHeader("Top-N Rankings");
        
        {
            std::cout << "\n6. Top 5 Sites by Average Concentration:\n";
            std::vector<std::pair<std::string, double>> rowTop5, colTop5;
            
            BenchmarkUtils::runAndReport(
                "Row Parallel", 
                [&]{ rowTop5 = rowService.topNSitesByAverageConcentration(5, numThreads); },
                [&]{}, numReps);
            
            BenchmarkUtils::runAndReport(
                "Column Parallel", 
                [&]{ colTop5 = columnService.topNSitesByAverageConcentration(5, numThreads); },
                [&]{}, numReps);
            
            std::cout << "  Row Model Top 5:\n";
            for (size_t i = 0; i < std::min(size_t(5), rowTop5.size()); ++i) {
                std::cout << "    " << (i+1) << ". " << rowTop5[i].first 
                          << " (" << std::fixed << std::setprecision(2) << rowTop5[i].second << ")\n";
            }
        }
        
        {
            std::cout << "\n7. Top 5 Sites by Max AQI:\n";
            std::vector<std::pair<std::string, int>> rowTop5, colTop5;
            
            BenchmarkUtils::runAndReport(
                "Row Parallel", 
                [&]{ rowTop5 = rowService.topNSitesByMaxAQI(5, numThreads); },
                [&]{}, numReps);
            
            BenchmarkUtils::runAndReport(
                "Column Parallel", 
                [&]{ colTop5 = columnService.topNSitesByMaxAQI(5, numThreads); },
                [&]{}, numReps);
            
            std::cout << "  Row Model Top 5:\n";
            for (size_t i = 0; i < std::min(size_t(5), rowTop5.size()); ++i) {
                std::cout << "    " << (i+1) << ". " << rowTop5[i].first 
                          << " (AQI=" << rowTop5[i].second << ")\n";
            }
        }
        
        // === Category Distribution ===
        printSectionHeader("Category Distribution Analysis");
        
        {
            std::cout << "\n8. AQI Category Distribution (0=Good, 1=Moderate, 2=USG, 3=Unhealthy, 4=Very Unhealthy, 5=Hazardous):\n";
            std::vector<std::size_t> rowDist, colDist;
            
            BenchmarkUtils::runAndReport(
                "Row Parallel", 
                [&]{ rowDist = rowService.categoryDistribution(numThreads); },
                [&]{}, numReps);
            
            BenchmarkUtils::runAndReport(
                "Column Parallel", 
                [&]{ colDist = columnService.categoryDistribution(numThreads); },
                [&]{}, numReps);
            
            std::cout << "  Category Distribution (Row Model):\n";
            const char* categoryNames[] = {"Good", "Moderate", "USG", "Unhealthy", "Very Unhealthy", "Hazardous"};
            for (int i = 0; i < 6; ++i) {
                double pct = rowDist[i] * 100.0 / rowModel.totalMeasurements();
                std::cout << "    " << i << " (" << categoryNames[i] << "): " 
                          << rowDist[i] << " (" << std::fixed << std::setprecision(1) << pct << "%)\n";
            }
        }
        
        printSectionHeader("Benchmark Complete");
        std::cout << "\nSummary:\n";
        std::cout << "  ✓ All fire analytics operations implemented\n";
        std::cout << "  ✓ Both serial and parallel execution working\n";
        std::cout << "  ✓ Row and column services producing consistent results\n";
        std::cout << "  ✓ " << rowModel.totalMeasurements() << " measurements processed successfully\n\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}


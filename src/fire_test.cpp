#include "../interface/fireRowModel.hpp"
#include <iostream>
#include <chrono>

int main() {
    try {
        FireRowModel fire_model;
        
        std::cout << "=== Fire Data Multi-threaded CSV Reading Test ===" << std::endl;
        
        // Test with fire data directory
        std::string fire_data_path = "data/fireData";
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Test parallel reading with 3 threads
        std::cout << "\n--- Testing Parallel Reading (3 threads) ---" << std::endl;
        fire_model.readFromDirectoryParallel(fire_data_path, 3);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "\n=== Results ===" << std::endl;
        std::cout << "Total sites: " << fire_model.siteCount() << std::endl;
        std::cout << "Total measurements: " << fire_model.totalMeasurements() << std::endl;
        std::cout << "Processing time: " << duration.count() << " ms" << std::endl;
        
        // Show some metadata
        std::cout << "\nParameters found: ";
        const auto& params = fire_model.parameters();
        for (size_t i = 0; i < params.size(); ++i) {
            std::cout << params[i];
            if (i < params.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        
        std::cout << "Agencies found: " << fire_model.agencies().size() << std::endl;
        
        // Show geographic bounds
        double min_lat, max_lat, min_lon, max_lon;
        fire_model.getGeographicBounds(min_lat, max_lat, min_lon, max_lon);
        std::cout << "Geographic bounds: (" << min_lat << ", " << min_lon << ") to (" 
                  << max_lat << ", " << max_lon << ")" << std::endl;
        
        // Test single-threaded fallback
        std::cout << "\n--- Testing Single-threaded Fallback ---" << std::endl;
        FireRowModel single_model;
        
        // Use just one date directory for quick test
        std::vector<std::string> test_files;
        test_files.push_back("data/fireData/20200810/20200810-01.csv");
        test_files.push_back("data/fireData/20200810/20200810-03.csv");
        
        single_model.readFromMultipleCSVParallel(test_files, 1); // Should use single thread
        
        std::cout << "Single-threaded test completed with " << single_model.totalMeasurements() 
                  << " measurements." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
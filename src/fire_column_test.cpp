#include "../interface/fireColumnModel.hpp"
#include <iostream>
#include <chrono>

int main() {
    std::cout << "=== Fire Column Model Test ===" << std::endl;
    
    FireColumnModel fireColumnModel;
    
    // Test directory path (adjust as needed)
    std::string dataDir = "data/FireData";
    
    // Test with different thread counts
    std::vector<int> threadCounts = {1, 4, 8};
    
    for (int threads : threadCounts) {
        std::cout << "\n--- Testing with " << threads << " thread(s) ---" << std::endl;
        
        FireColumnModel testModel;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            testModel.readFromDirectory(dataDir, threads);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            std::cout << "Processing completed in " << duration.count() << " ms" << std::endl;
            std::cout << "Total measurements: " << testModel.measurementCount() << std::endl;
            std::cout << "Total sites: " << testModel.siteCount() << std::endl;
            std::cout << "Unique parameters: " << testModel.uniqueParameters().size() << std::endl;
            std::cout << "Unique agencies: " << testModel.uniqueAgencies().size() << std::endl;
            
            // Show geographic bounds
            double min_lat, max_lat, min_lon, max_lon;
            testModel.getGeographicBounds(min_lat, max_lat, min_lon, max_lon);
            std::cout << "Geographic bounds: (" << min_lat << ", " << min_lon 
                      << ") to (" << max_lat << ", " << max_lon << ")" << std::endl;
            
            // Show datetime range
            const auto& timeRange = testModel.datetimeRange();
            if (!timeRange[0].empty() && !timeRange[1].empty()) {
                std::cout << "Datetime range: " << timeRange[0] << " to " << timeRange[1] << std::endl;
            }
            
            // Test some query operations
            if (!testModel.uniqueParameters().empty()) {
                std::string firstParam = *testModel.uniqueParameters().begin();
                auto paramIndices = testModel.getIndicesByParameter(firstParam);
                std::cout << "Parameter '" << firstParam << "' has " << paramIndices.size() << " measurements" << std::endl;
            }
            
            if (!testModel.uniqueSites().empty()) {
                std::string firstSite = *testModel.uniqueSites().begin();
                auto siteIndices = testModel.getIndicesBySite(firstSite);
                std::cout << "Site '" << firstSite << "' has " << siteIndices.size() << " measurements" << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
    
    std::cout << "\n=== Fire Column Model Test Complete ===" << std::endl;
    return 0;
}
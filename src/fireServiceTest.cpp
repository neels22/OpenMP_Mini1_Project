#include "../interface/fire_service_direct.hpp"
#include "../interface/fireRowModel.hpp"
#include "../interface/fireColumnModel.hpp"
#include <iostream>
#include <memory>

/**
 * @file fire_service_direct_test.cpp
 * @brief Simple test for the direct (non-virtual) fire analytics services
 * 
 * Tests the 4 core operations: maxAQI, minAQI, averageAQI, topNSitesByAverageConcentration
 * on both row-oriented and column-oriented fire data models without virtual interfaces.
 */

int main() {
    try {
        std::cout << "=== Fire Service Direct Analytics Test ===\n\n";
        
        // Create and load row model
        std::cout << "Loading fire data into row model...\n";
        FireRowModel rowModel;
        rowModel.readFromDirectoryParallel("data/fireData", 4); // Use 4 threads for loading
        
        // Create and load column model
        std::cout << "Loading fire data into column model...\n";
        FireColumnModel columnModel;
        columnModel.readFromDirectoryParallel("data/fireData", 4); // Use 4 threads for loading
        
        // Create services (no virtual interfaces)
        FireRowService rowService(&rowModel);
        FireColumnService columnService(&columnModel);
        
        std::cout << "\n=== Model Statistics ===\n";
        std::cout << "Row Model (" << rowService.getImplementationName() << "): " 
                  << rowService.totalMeasurementCount() << " measurements, " 
                  << rowService.uniqueSiteCount() << " sites\n";
        std::cout << "Column Model (" << columnService.getImplementationName() << "): " 
                  << columnService.totalMeasurementCount() << " measurements, " 
                  << columnService.uniqueSiteCount() << " sites\n";
        
        std::cout << "\n=== AQI Analytics (Serial) ===\n";
        
        // Test serial operations
        std::cout << "Max AQI:\n";
        std::cout << "  Row Model: " << rowService.maxAQI(1) << "\n";
        std::cout << "  Column Model: " << columnService.maxAQI(1) << "\n";
        
        std::cout << "Min AQI:\n";
        std::cout << "  Row Model: " << rowService.minAQI(1) << "\n";
        std::cout << "  Column Model: " << columnService.minAQI(1) << "\n";
        
        std::cout << "Average AQI:\n";
        std::cout << "  Row Model: " << rowService.averageAQI(1) << "\n";
        std::cout << "  Column Model: " << columnService.averageAQI(1) << "\n";
        
        std::cout << "\n=== AQI Analytics (Parallel - 4 threads) ===\n";
        
        // Test parallel operations
        std::cout << "Max AQI:\n";
        std::cout << "  Row Model: " << rowService.maxAQI(4) << "\n";
        std::cout << "  Column Model: " << columnService.maxAQI(4) << "\n";
        
        std::cout << "Min AQI:\n";
        std::cout << "  Row Model: " << rowService.minAQI(4) << "\n";
        std::cout << "  Column Model: " << columnService.minAQI(4) << "\n";
        
        std::cout << "Average AQI:\n";
        std::cout << "  Row Model: " << rowService.averageAQI(4) << "\n";
        std::cout << "  Column Model: " << columnService.averageAQI(4) << "\n";
        
        std::cout << "\n=== Top-5 Sites by Average Concentration (Serial) ===\n";
        
        // Test top-N operations
        auto rowTop5 = rowService.topNSitesByAverageConcentration(5, 1);
        auto columnTop5 = columnService.topNSitesByAverageConcentration(5, 1);
        
        std::cout << "Row Model Top-5:\n";
        for (std::size_t i = 0; i < rowTop5.size(); ++i) {
            std::cout << "  " << (i+1) << ". " << rowTop5[i].first 
                      << " (avg: " << rowTop5[i].second << ")\n";
        }
        
        std::cout << "Column Model Top-5:\n";
        for (std::size_t i = 0; i < columnTop5.size(); ++i) {
            std::cout << "  " << (i+1) << ". " << columnTop5[i].first 
                      << " (avg: " << columnTop5[i].second << ")\n";
        }
        
        std::cout << "\n=== Top-5 Sites by Average Concentration (Parallel - 4 threads) ===\n";
        
        // Test parallel top-N operations
        auto rowTop5Parallel = rowService.topNSitesByAverageConcentration(5, 4);
        auto columnTop5Parallel = columnService.topNSitesByAverageConcentration(5, 4);
        
        std::cout << "Row Model Top-5 (Parallel):\n";
        for (std::size_t i = 0; i < rowTop5Parallel.size(); ++i) {
            std::cout << "  " << (i+1) << ". " << rowTop5Parallel[i].first 
                      << " (avg: " << rowTop5Parallel[i].second << ")\n";
        }
        
        std::cout << "Column Model Top-5 (Parallel):\n";
        for (std::size_t i = 0; i < columnTop5Parallel.size(); ++i) {
            std::cout << "  " << (i+1) << ". " << columnTop5Parallel[i].first 
                      << " (avg: " << columnTop5Parallel[i].second << ")\n";
        }
        
        std::cout << "\n=== Verification: Results Should Match ===\n";
        
        // Verify that serial and parallel give same results
        bool maxMatch = (rowService.maxAQI(1) == rowService.maxAQI(4)) && 
                       (columnService.maxAQI(1) == columnService.maxAQI(4));
        bool minMatch = (rowService.minAQI(1) == rowService.minAQI(4)) && 
                       (columnService.minAQI(1) == columnService.minAQI(4));
        bool avgMatch = (std::abs(rowService.averageAQI(1) - rowService.averageAQI(4)) < 0.001) && 
                       (std::abs(columnService.averageAQI(1) - columnService.averageAQI(4)) < 0.001);
        
        std::cout << "Serial vs Parallel consistency:\n";
        std::cout << "  Max AQI: " << (maxMatch ? "✓ PASS" : "✗ FAIL") << "\n";
        std::cout << "  Min AQI: " << (minMatch ? "✓ PASS" : "✗ FAIL") << "\n";
        std::cout << "  Avg AQI: " << (avgMatch ? "✓ PASS" : "✗ FAIL") << "\n";
        
        std::cout << "\n=== Test Complete ===\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
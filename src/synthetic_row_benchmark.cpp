#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <functional>
#include <thread>
#include "../interface/populationModel.hpp"
#include "../interface/service.hpp"
#include "../interface/constants.hpp"
#include <filesystem>
#include <fstream>
#include <cstdlib>

using Clock = std::chrono::high_resolution_clock;

int main(int argc, char** argv) {
    // Defaults (can be overridden from CLI)
    std::size_t rows = Config::DEFAULT_SYNTHETIC_ROWS; // number of countries
    std::size_t years = Config::DEFAULT_SYNTHETIC_YEARS;    // number of year columns per row
    int repetitions = Config::DEFAULT_REPETITIONS;
    int threads = static_cast<int>(std::thread::hardware_concurrency()); 
    if (threads <= 0) threads = Config::DEFAULT_THREADS_FALLBACK;

    if (argc > 1) rows = static_cast<std::size_t>(std::stoull(argv[1]));
    if (argc > 2) years = static_cast<std::size_t>(std::stoull(argv[2]));
    if (argc > 3) repetitions = std::stoi(argv[3]);
    if (argc > 4) threads = std::stoi(argv[4]);

    std::cout << "Synthetic row-wise benchmark (CSV-driven): rows="<<rows<<" years="<<years<<" reps="<<repetitions<<" threads="<<threads<<"\n";

    // Build CSV file at data/PopulationData/population.csv
    // Ensure directory exists
    try {
        std::filesystem::create_directories("data/PopulationData");
    } catch (...) {
        // ignore
    }
    const std::string csvPath = "data/PopulationData/population_synthetic.csv";
    std::ofstream csv(csvPath);
    if (!csv) {
        std::cerr << "Failed to create CSV at "<<csvPath<<"\n";
        return 2;
    }

    // Header: first 4 columns then years
    csv << "Country Name,Country Code,Indicator Name,Indicator Code";
    for (std::size_t y = 0; y < years; ++y) csv << "," << (Config::DEFAULT_BASE_YEAR + static_cast<int>(y));
    csv << "\n";

    // Random generator
    std::mt19937_64 rng(Config::DEFAULT_RNG_SEED);
    std::uniform_int_distribution<long long> dist(0, 1000000);

    for (std::size_t i = 0; i < rows; ++i) {
        csv << "Country_" << i << ",C" << i << ",Indicator,I";
        for (std::size_t y = 0; y < years; ++y) csv << "," << dist(rng);
        csv << "\n";
    }
    csv.close();

    std::cout << "Wrote CSV to "<<csvPath<<" (size approx "<< (rows * years) <<" values)\n";

    // Now invoke the main app to exercise the full pipeline
    // Use positional args: repetitions and threads
    std::string cmd = std::string("./build/OpenMP_Mini1_Project_app ") + std::to_string(repetitions) + " " + std::to_string(threads);
    std::cout << "Running: "<<cmd<<"\n";
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::cerr << "App exited with code "<<rc<<"\n";
        return rc;
    }

    return 0;
}

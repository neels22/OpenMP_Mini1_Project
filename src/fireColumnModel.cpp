#include "../interface/fireColumnModel.hpp"
#include "../interface/utils.hpp"
#include "../interface/readcsv.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <omp.h>

// ============================================================================
// FireColumnModel Implementation
// ============================================================================

FireColumnModel::FireColumnModel() 
    : _min_latitude(0.0), _max_latitude(0.0), _min_longitude(0.0), _max_longitude(0.0),
      _bounds_initialized(false) {
    _datetime_range.resize(2);
}

FireColumnModel::~FireColumnModel() = default;

void FireColumnModel::readFromDirectory(const std::string& directoryPath, int numThreads) {
    if (numThreads <= 1) {
        // Serial processing
        auto csvFiles = getCSVFiles(directoryPath);
        for (const auto& file : csvFiles) {
            readFromCSV(file);
        }
    } else {
        // Parallel processing
        readFromDirectoryParallel(directoryPath, numThreads);
    }
}

void FireColumnModel::readFromDirectoryParallel(const std::string& directoryPath, int numThreads) {
    auto csvFiles = getCSVFiles(directoryPath);
    
    if (csvFiles.empty()) {
        std::cout << "No CSV files found in directory: " << directoryPath << std::endl;
        return;
    }
    
    std::cout << "Processing " << csvFiles.size() << " CSV files using " << numThreads << " threads..." << std::endl;
    
    // Create thread-local models and track files per thread
    std::vector<FireColumnModel> threadModels(numThreads);
    std::vector<int> filesPerThread(numThreads, 0);
    
    auto start_parallel = std::chrono::high_resolution_clock::now();
    
    // Use OpenMP to distribute files across threads
    #pragma omp parallel num_threads(numThreads)
    {
        int tid = omp_get_thread_num();
        
        #pragma omp for schedule(dynamic, 1)
        for (std::size_t i = 0; i < csvFiles.size(); ++i) {
            try {
                threadModels[tid].readFromCSV(csvFiles[i]);
                filesPerThread[tid]++;
            } catch (const std::exception& e) {
                #pragma omp critical
                {
                    std::cerr << "Error processing " << csvFiles[i] << ": " << e.what() << std::endl;
                }
            }
        }
    }
    
    auto end_parallel = std::chrono::high_resolution_clock::now();
    auto parallel_time = std::chrono::duration<double>(end_parallel - start_parallel).count();
    
    // Display thread summary
    std::cout << "\nThread Processing Summary:" << std::endl;
    for (int t = 0; t < numThreads; ++t) {
        std::cout << "Thread " << t << " processed " << filesPerThread[t] 
                  << " files, " << threadModels[t].measurementCount() << " measurements" << std::endl;
    }
    std::cout << "Parallel processing completed in " << std::fixed << std::setprecision(1) 
              << parallel_time << " seconds." << std::endl;
    
    // Merge phase
    auto start_merge = std::chrono::high_resolution_clock::now();
    for (int t = 0; t < numThreads; ++t) {
        if (threadModels[t].measurementCount() > 0) {
            mergeFromModel(threadModels[t]);
        }
    }
    auto end_merge = std::chrono::high_resolution_clock::now();
    auto merge_time = std::chrono::duration<double>(end_merge - start_merge).count();
    
    std::cout << "Merge phase completed in " << std::fixed << std::setprecision(1) 
              << merge_time << " seconds." << std::endl;
    std::cout << "Total processing time: " << std::fixed << std::setprecision(1) 
              << (parallel_time + merge_time) << " seconds." << std::endl;
    
    // Calculate and display parallel efficiency
    double theoretical_min_time = parallel_time / numThreads;
    double efficiency = (theoretical_min_time / parallel_time) * 100.0;
    std::cout << "Parallel efficiency: " << std::fixed << std::setprecision(1) 
              << efficiency << "%" << std::endl;
}

void FireColumnModel::readFromCSV(const std::string& filename) {
    CSVReader reader(filename);
    
    try {
        reader.open();
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to open CSV file " + filename + ": " + e.what());
    }
    
    std::vector<std::string> row;
    bool headerSkipped = false;
    
    while (reader.readRow(row)) {
        // Skip header row
        if (!headerSkipped) {
            headerSkipped = true;
            continue;
        }
        
        // Expect at least 13 columns for complete fire measurement data
        if (row.size() < 13) {
            continue; // Skip incomplete rows
        }
        
        try {
            // Parse row data (assuming standard fire data CSV format)
            double latitude = std::stod(row[0]);
            double longitude = std::stod(row[1]);
            std::string datetime = row[2];
            std::string parameter = row[3];
            double concentration = std::stod(row[4]);
            std::string unit = row[5];
            double raw_concentration = std::stod(row[6]);
            int aqi = std::stoi(row[7]);
            int category = std::stoi(row[8]);
            std::string site_name = row[9];
            std::string agency_name = row[10];
            std::string aqs_code = row[11];
            std::string full_aqs_code = row[12];
            
            insertMeasurement(latitude, longitude, datetime, parameter, concentration,
                            unit, raw_concentration, aqi, category, site_name,
                            agency_name, aqs_code, full_aqs_code);
            
        } catch (const std::exception& e) {
            // Skip rows with parsing errors
            continue;
        }
    }
    
    reader.close();
}

void FireColumnModel::insertMeasurement(double latitude, double longitude, const std::string& datetime,
                                       const std::string& parameter, double concentration, const std::string& unit,
                                       double raw_concentration, int aqi, int category,
                                       const std::string& site_name, const std::string& agency_name,
                                       const std::string& aqs_code, const std::string& full_aqs_code) {
    // Insert into columnar storage
    _latitudes.push_back(latitude);
    _longitudes.push_back(longitude);
    _datetimes.push_back(datetime);
    _parameters.push_back(parameter);
    _concentrations.push_back(concentration);
    _units.push_back(unit);
    _raw_concentrations.push_back(raw_concentration);
    _aqis.push_back(aqi);
    _categories.push_back(category);
    _site_names.push_back(site_name);
    _agency_names.push_back(agency_name);
    _aqs_codes.push_back(aqs_code);
    _full_aqs_codes.push_back(full_aqs_code);
    
    // Update indices and metadata
    std::size_t newIndex = _latitudes.size() - 1;
    updateIndices(newIndex);
    updateGeographicBounds(latitude, longitude);
    updateDatetimeRange(datetime);
    
    // Update unique sets
    _unique_sites.insert(site_name);
    _unique_parameters.insert(parameter);
    _unique_agencies.insert(agency_name);
}

void FireColumnModel::mergeFromModel(const FireColumnModel& other) {
    if (other.measurementCount() == 0) {
        return;
    }
    
    std::size_t currentSize = measurementCount();
    
    // Merge columnar data
    _latitudes.insert(_latitudes.end(), other._latitudes.begin(), other._latitudes.end());
    _longitudes.insert(_longitudes.end(), other._longitudes.begin(), other._longitudes.end());
    _datetimes.insert(_datetimes.end(), other._datetimes.begin(), other._datetimes.end());
    _parameters.insert(_parameters.end(), other._parameters.begin(), other._parameters.end());
    _concentrations.insert(_concentrations.end(), other._concentrations.begin(), other._concentrations.end());
    _units.insert(_units.end(), other._units.begin(), other._units.end());
    _raw_concentrations.insert(_raw_concentrations.end(), other._raw_concentrations.begin(), other._raw_concentrations.end());
    _aqis.insert(_aqis.end(), other._aqis.begin(), other._aqis.end());
    _categories.insert(_categories.end(), other._categories.begin(), other._categories.end());
    _site_names.insert(_site_names.end(), other._site_names.begin(), other._site_names.end());
    _agency_names.insert(_agency_names.end(), other._agency_names.begin(), other._agency_names.end());
    _aqs_codes.insert(_aqs_codes.end(), other._aqs_codes.begin(), other._aqs_codes.end());
    _full_aqs_codes.insert(_full_aqs_codes.end(), other._full_aqs_codes.begin(), other._full_aqs_codes.end());
    
    // Merge unique sets
    _unique_sites.insert(other._unique_sites.begin(), other._unique_sites.end());
    _unique_parameters.insert(other._unique_parameters.begin(), other._unique_parameters.end());
    _unique_agencies.insert(other._unique_agencies.begin(), other._unique_agencies.end());
    
    // Update indices for newly added measurements
    for (std::size_t i = currentSize; i < measurementCount(); ++i) {
        updateIndices(i);
    }
    
    // Merge geographic bounds
    if (other._bounds_initialized) {
        updateGeographicBounds(other._min_latitude, other._min_longitude);
        updateGeographicBounds(other._max_latitude, other._max_longitude);
    }
    
    // Merge datetime range
    if (!other._datetime_range[0].empty()) {
        updateDatetimeRange(other._datetime_range[0]);
    }
    if (!other._datetime_range[1].empty()) {
        updateDatetimeRange(other._datetime_range[1]);
    }
}

std::vector<std::size_t> FireColumnModel::getIndicesBySite(const std::string& siteName) const {
    auto it = _site_indices.find(siteName);
    return (it != _site_indices.end()) ? it->second : std::vector<std::size_t>{};
}

std::vector<std::size_t> FireColumnModel::getIndicesByParameter(const std::string& parameter) const {
    auto it = _parameter_indices.find(parameter);
    return (it != _parameter_indices.end()) ? it->second : std::vector<std::size_t>{};
}

std::vector<std::size_t> FireColumnModel::getIndicesByAqsCode(const std::string& aqsCode) const {
    auto it = _aqs_indices.find(aqsCode);
    return (it != _aqs_indices.end()) ? it->second : std::vector<std::size_t>{};
}

void FireColumnModel::getGeographicBounds(double& min_lat, double& max_lat, 
                                         double& min_lon, double& max_lon) const {
    if (_bounds_initialized) {
        min_lat = _min_latitude;
        max_lat = _max_latitude;
        min_lon = _min_longitude;
        max_lon = _max_longitude;
    } else {
        min_lat = max_lat = min_lon = max_lon = 0.0;
    }
}

void FireColumnModel::updateIndices(std::size_t index) {
    if (index >= _site_names.size()) return;
    
    // Update site indices
    _site_indices[_site_names[index]].push_back(index);
    
    // Update parameter indices
    _parameter_indices[_parameters[index]].push_back(index);
    
    // Update AQS code indices
    _aqs_indices[_aqs_codes[index]].push_back(index);
}

void FireColumnModel::updateGeographicBounds(double latitude, double longitude) {
    if (!_bounds_initialized) {
        _min_latitude = _max_latitude = latitude;
        _min_longitude = _max_longitude = longitude;
        _bounds_initialized = true;
    } else {
        _min_latitude = std::min(_min_latitude, latitude);
        _max_latitude = std::max(_max_latitude, latitude);
        _min_longitude = std::min(_min_longitude, longitude);
        _max_longitude = std::max(_max_longitude, longitude);
    }
}

void FireColumnModel::updateDatetimeRange(const std::string& datetime) {
    if (_datetime_range[0].empty() || datetime < _datetime_range[0]) {
        _datetime_range[0] = datetime;
    }
    if (_datetime_range[1].empty() || datetime > _datetime_range[1]) {
        _datetime_range[1] = datetime;
    }
}

std::vector<std::string> FireColumnModel::getCSVFiles(const std::string& directoryPath) const {
    std::vector<std::string> csvFiles;
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directoryPath)) {
            if (entry.is_regular_file()) {
                const std::string filename = entry.path().string();
                if (filename.size() >= 4 && 
                    filename.substr(filename.size() - 4) == ".csv") {
                    csvFiles.push_back(filename);
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error("Error accessing directory " + directoryPath + ": " + e.what());
    }
    
    // Sort files for consistent processing order
    std::sort(csvFiles.begin(), csvFiles.end());
    
    return csvFiles;
}
#include "../interface/fireRowModel.hpp"
#include "../interface/utils.hpp"
#include "../interface/readcsv.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <omp.h>

// ============================================================================
// FireMeasurement Implementation
// ============================================================================

FireMeasurement::FireMeasurement() 
    : _latitude(0.0), _longitude(0.0), _concentration(0.0), 
      _raw_concentration(0.0), _aqi(0), _category(0) {}

FireMeasurement::FireMeasurement(double latitude, double longitude, std::string datetime,
                               std::string parameter, double concentration, std::string unit,
                               double raw_concentration, int aqi, int category,
                               std::string site_name, std::string agency_name,
                               std::string aqs_code, std::string full_aqs_code)
    : _latitude(latitude), _longitude(longitude), _datetime(std::move(datetime)),
      _parameter(std::move(parameter)), _concentration(concentration), _unit(std::move(unit)),
      _raw_concentration(raw_concentration), _aqi(aqi), _category(category),
      _site_name(std::move(site_name)), _agency_name(std::move(agency_name)),
      _aqs_code(std::move(aqs_code)), _full_aqs_code(std::move(full_aqs_code)) {}

double FireMeasurement::latitude() const noexcept { return _latitude; }
double FireMeasurement::longitude() const noexcept { return _longitude; }
const std::string& FireMeasurement::datetime() const noexcept { return _datetime; }
const std::string& FireMeasurement::parameter() const noexcept { return _parameter; }
double FireMeasurement::concentration() const noexcept { return _concentration; }
const std::string& FireMeasurement::unit() const noexcept { return _unit; }
double FireMeasurement::rawConcentration() const noexcept { return _raw_concentration; }
int FireMeasurement::aqi() const noexcept { return _aqi; }
int FireMeasurement::category() const noexcept { return _category; }
const std::string& FireMeasurement::siteName() const noexcept { return _site_name; }
const std::string& FireMeasurement::agencyName() const noexcept { return _agency_name; }
const std::string& FireMeasurement::aqsCode() const noexcept { return _aqs_code; }
const std::string& FireMeasurement::fullAqsCode() const noexcept { return _full_aqs_code; }

// ============================================================================
// FireSiteData Implementation
// ============================================================================

FireSiteData::FireSiteData() = default;

FireSiteData::FireSiteData(std::string site_identifier, std::vector<FireMeasurement> measurements)
    : _site_identifier(std::move(site_identifier)), _measurements(std::move(measurements)) {}

const std::string& FireSiteData::siteIdentifier() const noexcept { return _site_identifier; }
const std::vector<FireMeasurement>& FireSiteData::measurements() const noexcept { return _measurements; }

const FireMeasurement& FireSiteData::getMeasurement(std::size_t index) const {
    if (index >= _measurements.size()) {
        throw std::out_of_range("Measurement index " + std::to_string(index) + 
                               " out of range [0, " + std::to_string(_measurements.size()) + ")");
    }
    return _measurements[index];
}

std::size_t FireSiteData::measurementCount() const noexcept { return _measurements.size(); }

void FireSiteData::addMeasurement(const FireMeasurement& measurement) {
    _measurements.push_back(measurement);
}

// ============================================================================
// FireRowModel Implementation
// ============================================================================

FireRowModel::FireRowModel() 
    : _total_measurements(0), _min_latitude(90.0), _max_latitude(-90.0),
      _min_longitude(180.0), _max_longitude(-180.0) {}

FireRowModel::~FireRowModel() = default;

// === Metadata Access Methods ===

const std::vector<std::string>& FireRowModel::siteNames() const noexcept { return _site_names; }
const std::vector<std::string>& FireRowModel::parameters() const noexcept { return _parameters; }
const std::vector<std::string>& FireRowModel::agencies() const noexcept { return _agencies; }
const std::vector<std::string>& FireRowModel::datetimeRange() const noexcept { return _datetime_range; }
const std::unordered_map<std::string, int>& FireRowModel::siteNameToIndex() const noexcept { return _site_name_to_index; }

// === Data Access Methods ===

std::size_t FireRowModel::siteCount() const noexcept { return _sites.size(); }
std::size_t FireRowModel::totalMeasurements() const noexcept { return _total_measurements; }

const FireSiteData& FireRowModel::siteAt(std::size_t idx) const {
    if (idx >= _sites.size()) {
        throw std::out_of_range("Site index " + std::to_string(idx) + 
                               " out of range [0, " + std::to_string(_sites.size()) + ")");
    }
    return _sites[idx];
}

const FireSiteData* FireRowModel::getBySiteName(const std::string& site_name) const noexcept {
    auto it = _site_name_to_index.find(site_name);
    if (it != _site_name_to_index.end()) {
        return &_sites[it->second];
    }
    return nullptr;
}

const FireSiteData* FireRowModel::getByAqsCode(const std::string& aqs_code) const noexcept {
    auto it = _aqs_code_to_index.find(aqs_code);
    if (it != _aqs_code_to_index.end()) {
        return &_sites[it->second];
    }
    return nullptr;
}

void FireRowModel::getGeographicBounds(double& min_lat, double& max_lat, 
                                     double& min_lon, double& max_lon) const noexcept {
    min_lat = _min_latitude;
    max_lat = _max_latitude;
    min_lon = _min_longitude;
    max_lon = _max_longitude;
}

// === Data Modification Methods ===

void FireRowModel::readFromCSV(const std::string& filename) {
    CSVReader reader(filename);
    try {
        reader.open();
    } catch (const std::exception& e) {
        throw std::runtime_error("Unable to open file: " + filename + " - " + e.what());
    }

    std::vector<std::string> row;
    std::size_t line_number = 0;
    
    while (reader.readRow(row)) {
        line_number++;
        
        // Skip empty rows
        if (row.empty()) {
            continue;
        }
        
        // Fire data CSV has no header, so process every row
        try {
            FireMeasurement measurement = parseCSVRow(row);
            insertMeasurement(measurement);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Error parsing line " << line_number 
                      << " in file " << filename << ": " << e.what() << std::endl;
            continue; // Skip malformed lines
        }
    }
    
    reader.close();
    std::cout << "Loaded " << line_number << " measurements from " << filename << std::endl;
}

void FireRowModel::readFromMultipleCSV(const std::vector<std::string>& filenames) {
    for (const auto& filename : filenames) {
        readFromCSV(filename);
    }
}

void FireRowModel::readFromMultipleCSVParallel(const std::vector<std::string>& filenames, int num_threads) {
    if (filenames.empty()) {
        std::cout << "No files to process." << std::endl;
        return;
    }
    
    // If single thread requested, use sequential processing
    if (num_threads <= 1) {
        std::cout << "Using single-threaded processing for " << filenames.size() << " files." << std::endl;
        readFromMultipleCSV(filenames);
        return;
    }
    
    // Limit threads to available files and reasonable maximum
    num_threads = std::min({num_threads, static_cast<int>(filenames.size()), omp_get_max_threads()});
    
    std::cout << "Using OpenMP parallel processing with " << num_threads 
              << " threads for " << filenames.size() << " files." << std::endl;
    
    // Create thread-local models (one per thread)
    std::vector<FireRowModel> thread_models(num_threads);
    
    // Track timing for performance analysis
    double start_time = omp_get_wtime();
    
    #pragma omp parallel num_threads(num_threads)
    {
        int thread_id = omp_get_thread_num();
        int files_processed = 0;
        
        #pragma omp for schedule(dynamic, 1) nowait
        for (size_t i = 0; i < filenames.size(); ++i) {
            try {
                thread_models[thread_id].readFromCSV(filenames[i]);
                files_processed++;
                
                #pragma omp critical(output)
                {
                    std::cout << "Thread " << thread_id << " completed: " 
                              << filenames[i] << " (file " << (i+1) << "/" << filenames.size() << ")" << std::endl;
                }
            } catch (const std::exception& e) {
                #pragma omp critical(error_output)
                {
                    std::cerr << "Thread " << thread_id << " error processing " 
                              << filenames[i] << ": " << e.what() << std::endl;
                }
            }
        }
        
        #pragma omp critical(thread_summary)
        {
            std::cout << "Thread " << thread_id << " processed " << files_processed << " files." << std::endl;
        }
    }
    // OpenMP barrier is implicit at end of parallel region
    
    double parallel_time = omp_get_wtime() - start_time;
    std::cout << "Parallel processing completed in " << parallel_time << " seconds." << std::endl;
    
    // Serial merge phase
    std::cout << "Starting merge phase..." << std::endl;
    double merge_start = omp_get_wtime();
    
    for (int t = 0; t < num_threads; ++t) {
        if (thread_models[t].totalMeasurements() > 0) {
            std::cout << "Merging " << thread_models[t].totalMeasurements() 
                      << " measurements from thread " << t << std::endl;
            mergeFromModel(thread_models[t]);
        }
    }
    
    double merge_time = omp_get_wtime() - merge_start;
    double total_time = omp_get_wtime() - start_time;
    
    std::cout << "Merge phase completed in " << merge_time << " seconds." << std::endl;
    std::cout << "Total processing time: " << total_time << " seconds." << std::endl;
    std::cout << "Parallel efficiency: " << (parallel_time / total_time * 100) << "%" << std::endl;
}

void FireRowModel::readFromDirectory(const std::string& directory_path) {
    std::vector<std::string> csv_files;
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".csv") {
                csv_files.push_back(entry.path().string());
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error("Error reading directory " + directory_path + ": " + e.what());
    }
    
    if (csv_files.empty()) {
        throw std::runtime_error("No CSV files found in directory: " + directory_path);
    }
    
    // Sort filenames for consistent ordering
    std::sort(csv_files.begin(), csv_files.end());
    
    std::cout << "Found " << csv_files.size() << " CSV files in " << directory_path << std::endl;
    readFromMultipleCSV(csv_files);
}

void FireRowModel::readFromDirectoryParallel(const std::string& directory_path, int num_threads) {
    std::vector<std::string> csv_files;
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".csv") {
                csv_files.push_back(entry.path().string());
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error("Error reading directory " + directory_path + ": " + e.what());
    }
    
    if (csv_files.empty()) {
        throw std::runtime_error("No CSV files found in directory: " + directory_path);
    }
    
    // Sort filenames for consistent ordering
    std::sort(csv_files.begin(), csv_files.end());
    
    std::cout << "Found " << csv_files.size() << " CSV files in " << directory_path << std::endl;
    readFromMultipleCSVParallel(csv_files, num_threads);
}

void FireRowModel::insertMeasurement(const FireMeasurement& measurement) {
    // Find or create site index
    int site_index = findOrCreateSiteIndex(measurement.siteName(), measurement.aqsCode());
    
    // Add measurement to the site
    _sites[site_index].addMeasurement(measurement);
    
    // Update metadata
    updateMetadata(measurement);
    
    // Update total count
    _total_measurements++;
}

void FireRowModel::clear() {
    _sites.clear();
    _site_names.clear();
    _parameters.clear();
    _agencies.clear();
    _datetime_range.clear();
    _site_name_to_index.clear();
    _aqs_code_to_index.clear();
    _total_measurements = 0;
    _min_latitude = 90.0;
    _max_latitude = -90.0;
    _min_longitude = 180.0;
    _max_longitude = -180.0;
}

// === Private Helper Methods ===

void FireRowModel::updateMetadata(const FireMeasurement& measurement) {
    // Update parameters
    if (std::find(_parameters.begin(), _parameters.end(), measurement.parameter()) == _parameters.end()) {
        _parameters.push_back(measurement.parameter());
    }
    
    // Update agencies
    if (std::find(_agencies.begin(), _agencies.end(), measurement.agencyName()) == _agencies.end()) {
        _agencies.push_back(measurement.agencyName());
    }
    
    // Update datetime range
    if (_datetime_range.empty()) {
        _datetime_range.push_back(measurement.datetime()); // min
        _datetime_range.push_back(measurement.datetime()); // max
    } else {
        if (measurement.datetime() < _datetime_range[0]) {
            _datetime_range[0] = measurement.datetime();
        }
        if (measurement.datetime() > _datetime_range[1]) {
            _datetime_range[1] = measurement.datetime();
        }
    }
    
    // Update geographic bounds
    _min_latitude = std::min(_min_latitude, measurement.latitude());
    _max_latitude = std::max(_max_latitude, measurement.latitude());
    _min_longitude = std::min(_min_longitude, measurement.longitude());
    _max_longitude = std::max(_max_longitude, measurement.longitude());
}

FireMeasurement FireRowModel::parseCSVRow(const std::vector<std::string>& tokens) const {
    if (tokens.size() != 13) {
        throw std::runtime_error("Expected 13 columns, got " + std::to_string(tokens.size()));
    }
    
    try {
        double latitude = std::stod(tokens[0]);
        double longitude = std::stod(tokens[1]);
        std::string datetime = tokens[2];
        std::string parameter = tokens[3];
        double concentration = std::stod(tokens[4]);
        std::string unit = tokens[5];
        double raw_concentration = std::stod(tokens[6]);
        int aqi = std::stoi(tokens[7]);
        int category = std::stoi(tokens[8]);
        std::string site_name = tokens[9];
        std::string agency_name = tokens[10];
        std::string aqs_code = tokens[11];
        std::string full_aqs_code = tokens[12];
        
        return FireMeasurement(latitude, longitude, datetime, parameter, concentration,
                             unit, raw_concentration, aqi, category, site_name,
                             agency_name, aqs_code, full_aqs_code);
    } catch (const std::invalid_argument& e) {
        throw std::runtime_error("Invalid numeric value in CSV row");
    } catch (const std::out_of_range& e) {
        throw std::runtime_error("Numeric value out of range in CSV row");
    }
}

int FireRowModel::findOrCreateSiteIndex(const std::string& site_name, const std::string& aqs_code) {
    // Try to find by site name first
    auto name_it = _site_name_to_index.find(site_name);
    if (name_it != _site_name_to_index.end()) {
        return name_it->second;
    }
    
    // Try to find by AQS code
    auto aqs_it = _aqs_code_to_index.find(aqs_code);
    if (aqs_it != _aqs_code_to_index.end()) {
        return aqs_it->second;
    }
    
    // Create new site
    int new_index = static_cast<int>(_sites.size());
    _sites.emplace_back(site_name, std::vector<FireMeasurement>());
    _site_names.push_back(site_name);
    _site_name_to_index[site_name] = new_index;
    _aqs_code_to_index[aqs_code] = new_index;
    
    return new_index;
}

void FireRowModel::mergeFromModel(const FireRowModel& other) {
    // Merge all measurements from the other model
    for (const auto& site : other._sites) {
        for (const auto& measurement : site.measurements()) {
            insertMeasurement(measurement);  // Reuses existing deduplication and indexing logic
        }
    }
}
#include "../interface/parallel_csv_loader.hpp"
#include "../interface/datetime_utils.hpp"
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <omp.h>

namespace AirQuality {

FileLoadResult ParallelCSVLoader::loadFile(const std::string& filepath) {
    auto start = std::chrono::high_resolution_clock::now();
    
    FileLoadResult result;
    result.filename = filepath;
    result.success = false;
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        result.errorMsg = "Cannot open file: " + filepath;
        return result;
    }
    
    std::string line;
    size_t lineNum = 0;
    size_t parseErrors = 0;
    
    while (std::getline(file, line)) {
        lineNum++;
        
        if (line.empty()) continue;
        
        // Skip lines that look like headers (contain "Latitude" or "DateTime")
        if (line.find("Latitude") != std::string::npos || 
            line.find("DateTime") != std::string::npos) {
            continue;
        }
        
        try {
            Record record = parseLine(line);
            if (record.isValid()) {
                result.records.push_back(std::move(record));
            } else {
                parseErrors++;
            }
        } catch (const std::exception& e) {
            parseErrors++;
            // Don't fail on individual line errors - continue processing
            if (parseErrors == 1) {
                result.errorMsg = "Parse errors starting at line " + std::to_string(lineNum);
            }
        }
    }
    
    file.close();
    
    auto end = std::chrono::high_resolution_clock::now();
    result.loadTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
    result.recordCount = result.records.size();
    result.success = true;
    
    // Append error summary if there were issues
    if (parseErrors > 0) {
        result.errorMsg += " (total errors: " + std::to_string(parseErrors) + ")";
    }
    
    return result;
}

std::vector<FileLoadResult> ParallelCSVLoader::loadSequential(
    const std::vector<std::string>& filepaths) {
    
    std::vector<FileLoadResult> results;
    results.reserve(filepaths.size());
    
    // Load files one by one
    for (const auto& filepath : filepaths) {
        results.push_back(loadFile(filepath));
    }
    
    return results;
}

std::vector<FileLoadResult> ParallelCSVLoader::loadParallel(
    const std::vector<std::string>& filepaths,
    int numThreads) {
    
    // Pre-allocate results vector
    std::vector<FileLoadResult> results(filepaths.size());
    
    // KEY: Parallel file loading with OpenMP
    // Each thread loads a different file - no locks needed!
    // schedule(dynamic) handles varying file sizes efficiently
    #pragma omp parallel for num_threads(numThreads) schedule(dynamic)
    for (size_t i = 0; i < filepaths.size(); i++) {
        results[i] = loadFile(filepaths[i]);
    }
    
    return results;
}

std::vector<std::string> ParallelCSVLoader::scanDirectory(const std::string& directory) {
    std::vector<std::string> csvFiles;
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".csv") {
                csvFiles.push_back(entry.path().string());
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        // Directory doesn't exist or can't be read
        return csvFiles;  // Return empty vector
    }
    
    // Sort files for consistent ordering
    std::sort(csvFiles.begin(), csvFiles.end());
    
    return csvFiles;
}

std::vector<std::string> ParallelCSVLoader::scanDirectoryPattern(
    const std::string& directory,
    const std::string& pattern) {
    
    auto allFiles = scanDirectory(directory);
    std::vector<std::string> matchingFiles;
    
    for (const auto& file : allFiles) {
        std::filesystem::path p(file);
        std::string filename = p.filename().string();
        
        // Simple pattern matching (contains)
        if (filename.find(pattern) != std::string::npos) {
            matchingFiles.push_back(file);
        }
    }
    
    return matchingFiles;
}

Record ParallelCSVLoader::parseLine(const std::string& line) {
    auto fields = splitCSV(line);
    
    // Expected format (13 fields):
    // 0: Latitude, 1: Longitude, 2: DateTime, 3: Pollutant, 4: Value, 5: Unit,
    // 6: AQI, 7: AQICategory, 8: QualityFlag, 9: Location, 10: Agency,
    // 11: SiteID1, 12: SiteID2
    
    if (fields.size() < 13) {
        throw std::runtime_error("Insufficient fields in CSV line");
    }
    
    Record record;
    
    try {
        // Parse geographic coordinates
        record.latitude = std::stod(fields[0]);
        record.longitude = std::stod(fields[1]);
        
        // Parse datetime
        record.dateTimeStr = fields[2];
        record.timestamp = DateTimeUtils::parseISO8601(fields[2]);
        
        // Pollutant information
        record.pollutant = fields[3];
        record.value = std::stod(fields[4]);
        record.unit = fields[5];
        
        // AQI information
        record.aqi = std::stod(fields[6]);
        record.aqiCategory = std::stoi(fields[7]);
        record.qualityFlag = std::stoi(fields[8]);
        
        // Station metadata
        record.location = fields[9];
        record.agency = fields[10];
        record.siteId1 = fields[11];
        record.siteId2 = fields[12];
        
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Parse error: ") + e.what());
    }
    
    return record;
}

std::vector<std::string> ParallelCSVLoader::splitCSV(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;
    
    for (size_t i = 0; i < line.length(); i++) {
        char c = line[i];
        
        if (c == '"') {
            // Toggle quote mode
            inQuotes = !inQuotes;
        } else if (c == ',' && !inQuotes) {
            // Field separator outside quotes
            fields.push_back(unquote(trim(current)));
            current.clear();
        } else {
            // Regular character
            current += c;
        }
    }
    
    // Add last field
    fields.push_back(unquote(trim(current)));
    
    return fields;
}

std::string ParallelCSVLoader::trim(const std::string& str) {
    if (str.empty()) return str;
    
    size_t start = 0;
    size_t end = str.length() - 1;
    
    // Trim leading whitespace
    while (start < str.length() && std::isspace(static_cast<unsigned char>(str[start]))) {
        start++;
    }
    
    // Trim trailing whitespace
    while (end > start && std::isspace(static_cast<unsigned char>(str[end]))) {
        end--;
    }
    
    return str.substr(start, end - start + 1);
}

std::string ParallelCSVLoader::unquote(const std::string& str) {
    if (str.length() >= 2 && str.front() == '"' && str.back() == '"') {
        return str.substr(1, str.length() - 2);
    }
    return str;
}

} // namespace AirQuality


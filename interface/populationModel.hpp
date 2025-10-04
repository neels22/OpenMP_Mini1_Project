#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <unordered_map>

// Represents one country's population values across a sequence of years
class PopulationRow {
private:
    std::string _country;
    std::vector<long long> _year_population;

public:
    PopulationRow();
    PopulationRow(std::string country, std::vector<long long> year_population);

    // Getters
    const std::string& country() const noexcept;
    const std::vector<long long>& yearPopulation() const noexcept;
    long long getPopulationForYear(std::size_t yearIndex) const;
    std::size_t yearCount() const noexcept;
};

class PopulationModel {
private:
    std::vector<PopulationRow> _rows;
    std::vector<std::string> _countryNames;
    std::vector<std::string> _countriesCode;
    std::vector<std::string> _indicatorNames;
    std::vector<std::string> _indicatorCodes;
    std::vector<long long> _years;
    std::unordered_map<std::string, int> _countryCodeToRowIndex;
    std::unordered_map<long long, int> _yearToIndex;
    std::unordered_map<std::string,std::string> _countryNameToCountryCode;

public:
    PopulationModel();
    ~PopulationModel();

    // Basic information accessors
    const std::vector<std::string>& countryNames() const noexcept;
    const std::vector<std::string>& countriesCode() const noexcept;
    const std::vector<std::string>& indicatorNames() const noexcept;
    const std::vector<std::string>& indicatorCodes() const noexcept;
    const std::vector<long long>& years() const noexcept;

    const std::unordered_map<std::string, int>& countryNameToIndex() const noexcept;
    const std::unordered_map<long long, int>& yearToIndex() const noexcept;

    // Convenience accessors
    std::size_t rowCount() const noexcept;
    const PopulationRow& rowAt(std::size_t idx) const;

    // Find a row by country name. Returns pointer (nullptr if not found).
    const PopulationRow* getByCountry(const std::string& country) const noexcept;

    // Set the years vector (only if rows are empty)
    bool setYears(std::vector<long long> years);
    void readFromCSV(const std::string& filename);
    // Insert a new entry (append)
    void insertNewEntry(std::string& county, std::string& contry_code, std::string& indicator_name, std::string& indicator_code, std::vector<long long>& year_population);
};

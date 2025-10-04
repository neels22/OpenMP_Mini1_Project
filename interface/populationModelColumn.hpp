#pragma once

#include <string>
#include <vector>
#include <unordered_map>

// Columnar storage: each year is a column (vector of country populations)
class PopulationModelColumn {
private:
    std::vector<std::string> _countryNames;
    std::vector<std::string> _countriesCode;
    std::vector<std::string> _indicatorNames;
    std::vector<std::string> _indicatorCodes;
    std::vector<long long> _years;

    // Data: one vector per year, each vector holds populations for all countries in insertion order
    std::vector<std::vector<long long>> _columns;

    std::unordered_map<std::string, int> _countryNameToIndex;
    std::unordered_map<std::string, std::string> _countryNameToCountryCode;
    std::unordered_map<long long, int> _yearToIndex;

public:
    PopulationModelColumn();
    ~PopulationModelColumn();

    // Basic accessors
    const std::vector<std::string>& countryNames() const noexcept;
    const std::vector<std::string>& countriesCode() const noexcept;
    const std::vector<std::string>& indicatorNames() const noexcept;
    const std::vector<std::string>& indicatorCodes() const noexcept;
    const std::vector<long long>& years() const noexcept;

    const std::unordered_map<std::string, int>& countryNameToIndex() const noexcept;
    const std::unordered_map<long long, int>& yearToIndex() const noexcept;

    std::size_t rowCount() const noexcept; // number of countries
    std::size_t yearCount() const noexcept; // number of years

    // Insert a new country row (append). Accepts same args as row model for convenience.
    void insertNewEntry(std::string country, std::string country_code, std::string indicator_name, std::string indicator_code, std::vector<long long> year_population);

    // Set the years vector (must be called before inserting rows)
    bool setYears(std::vector<long long> years);

    // Read CSV file and populate the columnar model. Expects the same CSV layout as the row model:
    // Country Name, Country Code, Indicator Name, Indicator Code, <year columns...>
    void readFromCSV(const std::string& filename);

    // Get population by country index and year index
    long long getPopulationForCountryYear(std::size_t countryIndex, std::size_t yearIndex) const;

    // Find by country name
    int countryNameIndex(const std::string& country) const noexcept;
};

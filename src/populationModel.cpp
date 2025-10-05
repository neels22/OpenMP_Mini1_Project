#include "../interface/populationModel.hpp"

#include "../interface/readcsv.hpp"
#include "../interface/utils.hpp"
#include <stdexcept>
#include <sstream>
#include <string>
#include <utility>
#include <iostream>

// PopulationRow implementations
PopulationRow::PopulationRow() = default;
PopulationRow::PopulationRow(std::string country, std::vector<long long> year_population)
    : _country(std::move(country)), _year_population(std::move(year_population)) {}

const std::string& PopulationRow::country() const noexcept { return _country; }
const std::vector<long long>& PopulationRow::yearPopulation() const noexcept { return _year_population; }
long long PopulationRow::getPopulationForYear(std::size_t yearIndex) const {
    if (yearIndex >= _year_population.size()) throw std::out_of_range("Year index out of range");
    return _year_population[yearIndex];
}
std::size_t PopulationRow::yearCount() const noexcept { return _year_population.size(); }

// PopulationModel implementations
PopulationModel::PopulationModel() = default;
PopulationModel::~PopulationModel() = default;

const std::vector<std::string>& PopulationModel::countryNames() const noexcept { return _countryNames; }
const std::vector<std::string>& PopulationModel::countriesCode() const noexcept { return _countriesCode; }
const std::vector<std::string>& PopulationModel::indicatorNames() const noexcept { return _indicatorNames; }
const std::vector<std::string>& PopulationModel::indicatorCodes() const noexcept { return _indicatorCodes; }
const std::vector<long long>& PopulationModel::years() const noexcept { return _years; }

// Build a temporary per-instance map from country name to row index using the header-provided maps.
const std::unordered_map<std::string, int>& PopulationModel::countryNameToIndex() const noexcept {
    // cache built per-instance in a thread_local variable
    thread_local std::unordered_map<std::string, int> cache;
    cache.clear();
    for (const auto& kv : _countryNameToCountryCode) {
        const std::string& cname = kv.first;
        const std::string& code = kv.second;
        auto it = _countryCodeToRowIndex.find(code);
        if (it != _countryCodeToRowIndex.end()) cache[cname] = it->second;
    }
    return cache;
}

const std::unordered_map<long long, int>& PopulationModel::yearToIndex() const noexcept { return _yearToIndex; }

std::size_t PopulationModel::rowCount() const noexcept { return _rows.size(); }
const PopulationRow& PopulationModel::rowAt(std::size_t idx) const { return _rows.at(idx); }

const PopulationRow* PopulationModel::getByCountry(const std::string& country) const noexcept {
    const auto &map = countryNameToIndex();
    auto it = map.find(country);
    if (it == map.end()) return nullptr;
    return &_rows[it->second];
}

bool PopulationModel::setYears(std::vector<long long> years) {
    if (!_rows.empty()) return false; // Cannot set years if rows already exist
    _years = std::move(years);
    _yearToIndex.clear();
    for (std::size_t i = 0; i < _years.size(); ++i) {
        _yearToIndex[_years[i]] = static_cast<int>(i);
    }
    return true;
}

// CSV reading is implemented in service.cpp when needed

void PopulationModel::insertNewEntry(std::string country, std::string contry_code, std::string indicator_name, std::string indicator_code, std::vector<long long> year_population)
{
    _countriesCode.push_back(std::move(contry_code));
    _indicatorNames.push_back(std::move(indicator_name));
    _indicatorCodes.push_back(std::move(indicator_code));
    PopulationRow newRow(std::move(country), std::move(year_population));
    std::size_t idx = _rows.size();
    _rows.push_back(std::move(newRow));
    _countryNames.push_back(_rows.back().country());
    // maintain the code->row mapping and name->code mapping
    _countryCodeToRowIndex[_countriesCode.back()] = static_cast<int>(idx);
    _countryNameToCountryCode[_countryNames.back()] = _countriesCode.back();
}

void PopulationModel::readFromCSV(const std::string& filename) {
    CSVReader reader(filename);
    try { 
        reader.open();
    } catch (const std::exception& e) {
        std::cerr << "Failed to open CSV: " << e.what() << "\n"; return;
    }
    std::vector<std::string> row;
    bool headerRead = false;
    std::vector<long long> yearsLocal;
    while (reader.readRow(row)) {
        if (!headerRead) {
            for (std::size_t i = 4; i < row.size(); ++i) {
                if (row[i].empty()) continue;
                yearsLocal.push_back(Utils::parseLongOrZero(row[i]));
            }
            setYears(yearsLocal);
            headerRead = true;
            continue;
        }
        if (row.size() < 5) continue;
        std::string country = row[0];
        std::string code = row[1];
        std::string iname = row[2];
        std::string icode = row[3];
        std::vector<long long> pops;
        pops.reserve(_years.size());
        for (std::size_t i = 4; i < row.size(); ++i) {
            if (row[i].empty()) pops.push_back(0);
            else pops.push_back(Utils::parseLongOrZero(row[i]));
        }
        insertNewEntry(country, code, iname, icode, pops);
    }
    reader.close();
}

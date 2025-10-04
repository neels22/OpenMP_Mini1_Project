#include "../interface/populationModelColumn.hpp"
#include "../interface/readcsv.hpp"
#include "../interface/constants.hpp"
#include "../interface/utils.hpp"
#include <string>
#include <iostream>

PopulationModelColumn::PopulationModelColumn() = default;
PopulationModelColumn::~PopulationModelColumn() = default;

const std::vector<std::string>& PopulationModelColumn::countryNames() const noexcept { return _countryNames; }
const std::vector<std::string>& PopulationModelColumn::countriesCode() const noexcept { return _countriesCode; }
const std::vector<std::string>& PopulationModelColumn::indicatorNames() const noexcept { return _indicatorNames; }
const std::vector<std::string>& PopulationModelColumn::indicatorCodes() const noexcept { return _indicatorCodes; }
const std::vector<long long>& PopulationModelColumn::years() const noexcept { return _years; }

const std::unordered_map<std::string, int>& PopulationModelColumn::countryNameToIndex() const noexcept { return _countryNameToIndex; }
const std::unordered_map<long long, int>& PopulationModelColumn::yearToIndex() const noexcept { return _yearToIndex; }

std::size_t PopulationModelColumn::rowCount() const noexcept { return _countryNames.size(); }
std::size_t PopulationModelColumn::yearCount() const noexcept { return _years.size(); }

bool PopulationModelColumn::setYears(std::vector<long long> years) {
    if (! _countryNames.empty()) return false; // only allowed when empty
    _years = std::move(years);
    _columns.clear();
    _columns.resize(_years.size());
    for (auto &col : _columns) col.reserve(Config::DEFAULT_COLUMN_RESERVE_SIZE);
    _yearToIndex.clear();
    for (std::size_t i = 0; i < _years.size(); ++i) _yearToIndex[_years[i]] = static_cast<int>(i);
    return true;
}

void PopulationModelColumn::insertNewEntry(std::string country, std::string country_code, std::string indicator_name, std::string indicator_code, std::vector<long long> year_population) {
    // store metadata (move into containers)
    _countryNames.push_back(std::move(country));
    _countriesCode.push_back(std::move(country_code));
    _indicatorNames.push_back(std::move(indicator_name));
    _indicatorCodes.push_back(std::move(indicator_code));
    int idx = static_cast<int>(_countryNames.size() - 1);
    _countryNameToIndex[_countryNames.back()] = idx;
    _countryNameToCountryCode[_countryNames.back()] = _countriesCode.back();

    // ensure columns sized
    std::size_t nYears = _years.size();
    if (_columns.size() < nYears) _columns.resize(nYears);

    // append values to each column; if the provided vector is shorter, pad with zeros
    for (std::size_t y = 0; y < nYears; ++y) {
        long long v = 0;
        if (y < year_population.size()) v = year_population[y];
        _columns[y].push_back(v);
    }
}

long long PopulationModelColumn::getPopulationForCountryYear(std::size_t countryIndex, std::size_t yearIndex) const {
    if (yearIndex >= _columns.size() || countryIndex >= rowCount()) return 0;
    return _columns[yearIndex][countryIndex];
}

int PopulationModelColumn::countryNameIndex(const std::string& country) const noexcept {
    auto it = _countryNameToIndex.find(country);
    if (it == _countryNameToIndex.end()) return -1;
    return it->second;
}

void PopulationModelColumn::readFromCSV(const std::string& filename) {
    CSVReader reader(filename);
    try { reader.open(); } catch (const std::exception& e) { std::cerr << "Failed to open CSV: " << e.what() << "\n"; return; }
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

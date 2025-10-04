#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <algorithm>
#include <functional>
#include "../interface/populationModel.hpp"
#include "../interface/service.hpp"
#include "../interface/readcsv.hpp"

#include <cctype>
#include <iomanip>
#include <thread>
#include <thread>
#include <cstdlib>

static long long parseLongOrZero(const std::string& s) {
    try {
        // allow leading/trailing spaces
        size_t idx = 0;
        long long v = std::stoll(s, &idx);
        return v;
    } catch (...) {
        return 0;
    }
}

using Clock = std::chrono::high_resolution_clock;

// Returns elapsed time in microseconds
static double time_call(const std::function<void()>& f) {
    auto t0 = Clock::now();
    f();
    auto t1 = Clock::now();
    std::chrono::duration<double, std::micro> d = t1 - t0;
    return d.count();
}

// Run `f` `runs` times, return vector of elapsed ms
static std::vector<double> time_call_multi(const std::function<void()>& f, int runs) {
    std::vector<double> res;
    res.reserve(static_cast<std::size_t>(runs));
    for (int i = 0; i < runs; ++i) res.push_back(time_call(f));
    return res;
}

static double median(std::vector<double> v) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    size_t m = v.size()/2;
    if (v.size() % 2 == 1) return v[m];
    return 0.5 * (v[m-1] + v[m]);
}

static double stddev(const std::vector<double>& v) {
    if (v.size() < 2) return 0.0;
    double mean = 0.0;
    for (double x : v) mean += x;
    mean /= static_cast<double>(v.size());
    double s = 0.0;
    for (double x : v) s += (x - mean) * (x - mean);
    return std::sqrt(s / static_cast<double>(v.size()-1));
}

int main(int argc, char** argv) {
    const char* envCsv = std::getenv("CSV_PATH");
    const std::string csvPath = envCsv ? std::string(envCsv) : std::string("data/PopulationData/population.csv");

    // Read CSV
    CSVReader reader(csvPath);
    try { reader.open(); } catch (const std::exception& e) { std::cerr << "Failed to open CSV: " << e.what() << "\n"; return 1; }

    PopulationModel model;
    std::vector<std::string> row;
    bool headerRead = false;
    std::vector<long long> years;
    while (reader.readRow(row)) {
        if (!headerRead) {
            // header contains year labels starting at index 4
            for (size_t i = 4; i < row.size(); ++i) {
                if (row[i].empty()) continue;
                years.push_back(parseLongOrZero(row[i]));
            }
            model.setYears(years);
            headerRead = true;
            continue;
        }
        if (row.size() < 5) continue;
        std::string country = row[0];
        std::string code = row[1];
        std::string iname = row[2];
        std::string icode = row[3];
        std::vector<long long> pops;
        pops.reserve(years.size());
        for (size_t i = 4; i < row.size(); ++i) {
            if (row[i].empty()) pops.push_back(0);
            else pops.push_back(parseLongOrZero(row[i]));
        }
        model.insertNewEntry(country, code, iname, icode, pops);
    }
    reader.close();

    PopulationModelService svc(&model);

    // Choose a mid-year for per-year ops
    if (years.empty()) { std::cerr << "No year columns found in CSV\n"; return 1; }
    int midYear = static_cast<int>(years[years.size()/2]);
    std::string sampleCountry = model.countryNames().empty() ? std::string() : model.countryNames()[0];

    std::cout << "Rows: " << model.rowCount() << " Years: " << model.years().size() << "\n";

    // Choose number of threads for the parallel run (use hardware concurrency if available)
    int parallelThreads = static_cast<int>(std::thread::hardware_concurrency());
    if (parallelThreads <= 0) parallelThreads = 4;

    // Number of repetitions per measurement (use 5 by default)
    int repetitions = 5;

    auto print_usage = [&](const char* prog){
        std::cout << "Usage: " << prog << " [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  -h, --help           Show this help message and exit\n";
        std::cout << "  -r N, --reps N       Number of repetitions per measurement (default 5)\n";
        std::cout << "  -t N, --threads N    Number of threads to use for parallel runs (default = hardware)\n";
        std::cout << "\nExamples:\n";
        std::cout << "  # run 5 repetitions and auto thread count\n";
        std::cout << "  " << prog << " -r 5\n";
        std::cout << "  # run 10 repetitions with 2 threads\n";
        std::cout << "  " << prog << " -r 10 -t 2\n";
    };

    // Parse CLI args: support flags and simple positional number for repetitions for backward compatibility
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-h" || a == "--help") { print_usage(argv[0]); return 0; }
        if (a == "-r" || a == "--reps") {
            if (i + 1 < argc) { try { repetitions = std::stoi(argv[++i]); if (repetitions < 1) repetitions = 1; } catch(...) { repetitions = 5; } }
            continue;
        }
        if (a.rfind("--reps=", 0) == 0) { try { repetitions = std::stoi(a.substr(7)); } catch(...) { repetitions = 5; } continue; }
        if (a == "-t" || a == "--threads") {
            if (i + 1 < argc) { try { int t = std::stoi(argv[++i]); if (t > 0) parallelThreads = t; } catch(...) { } }
            continue;
        }
        if (a.rfind("--threads=", 0) == 0) { try { int t = std::stoi(a.substr(10)); if (t > 0) parallelThreads = t; } catch(...) { } continue; }
        // Backward-compatible positional: first numeric arg is repetitions, second numeric is threads
        try {
            int v = std::stoi(a);
            if (repetitions == 5) { repetitions = v > 0 ? v : 5; continue; }
            int t = v; if (t > 0) parallelThreads = t;
        } catch(...) {
            // ignore unknown token
        }
    }

    // Run benchmarks for each API
    std::cout << std::fixed << std::setprecision(3);
    auto run_and_report = [&](const std::string &label, const std::function<void()> &serialFn, const std::function<void()> &parallelFn, const std::function<void()> &readResult){
        auto svec = time_call_multi(serialFn, repetitions);
        auto pvec = time_call_multi(parallelFn, repetitions);
        double sm = median(svec), pm = median(pvec);
        double ss = stddev(svec), ps = stddev(pvec);
        (void)readResult; // optional capture for retrieving last-run results if needed
        std::cout << label << ": serial_t_median=" << sm << " us stddev=" << ss << ", parallel_t_median=" << pm << " us stddev=" << ps << "\n";
    };

    {
        long long s=0,p=0;
        run_and_report("sumPopulationForYear",
                       [&]{ s = svc.sumPopulationForYear(midYear, 1); },
                       [&]{ p = svc.sumPopulationForYear(midYear, parallelThreads); },
                       [&]{});
        std::cout << "  -> values: serial=" << s << " parallel=" << p << "\n";
    }

    {
        double s=0,p=0;
        run_and_report("averagePopulationForYear",
                       [&]{ s = svc.averagePopulationForYear(midYear, 1); },
                       [&]{ p = svc.averagePopulationForYear(midYear, parallelThreads); },
                       [&]{});
        std::cout << "  -> values: serial=" << s << " parallel=" << p << "\n";
    }

    {
        long long s=0,p=0;
        run_and_report("maxPopulationForYear",
                       [&]{ s = svc.maxPopulationForYear(midYear, 1); },
                       [&]{ p = svc.maxPopulationForYear(midYear, parallelThreads); },
                       [&]{});
        std::cout << "  -> values: serial=" << s << " parallel=" << p << "\n";
    }

    {
        long long s=0,p=0;
        run_and_report("minPopulationForYear",
                       [&]{ s = svc.minPopulationForYear(midYear, 1); },
                       [&]{ p = svc.minPopulationForYear(midYear, parallelThreads); },
                       [&]{});
        std::cout << "  -> values: serial=" << s << " parallel=" << p << "\n";
    }

    // top N
    {
        std::vector<std::pair<std::string,long long>> svec, tvec;
        run_and_report("topNCountriesByPopulationInYear",
                       [&]{ svec = svc.topNCountriesByPopulationInYear(midYear, 10, 1); },
                       [&]{ tvec = svc.topNCountriesByPopulationInYear(midYear, 10, parallelThreads); },
                       [&]{});
        std::cout << "  -> counts: serial_count=" << svec.size() << " parallel_count=" << tvec.size() << "\n";
    }

    // population for a sample country
    if (!sampleCountry.empty()) {
        long long s=0,p=0;
        run_and_report("populationForCountryInYear",
                       [&]{ s = svc.populationForCountryInYear(sampleCountry, midYear, 1); },
                       [&]{ p = svc.populationForCountryInYear(sampleCountry, midYear, parallelThreads); },
                       [&]{});
        std::cout << "  -> values: serial=" << s << " parallel=" << p << "\n";

        std::vector<long long> over;
        run_and_report("poputationOverYearsForCountry",
                       [&]{ over = svc.poputationOverYearsForCountry(sampleCountry, static_cast<int>(years.front()), static_cast<int>(years.back()), 1); },
                       [&]{ (void)svc.poputationOverYearsForCountry(sampleCountry, static_cast<int>(years.front()), static_cast<int>(years.back()), parallelThreads); },
                       [&]{});
        std::cout << "  -> len="<<over.size()<<"\n";
    }

    return 0;
}

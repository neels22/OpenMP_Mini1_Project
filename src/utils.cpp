#include "../interface/utils.hpp"
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace Utils {
    long long parseLongOrZero(const std::string& s) {
        try {
            size_t idx = 0;
            long long v = std::stoll(s, &idx);
            return v;
        } catch (...) {
            return 0;
        }
    }

    double timeCall(const std::function<void()>& f) {
        auto t0 = Clock::now();
        f();
        auto t1 = Clock::now();
        std::chrono::duration<double, std::micro> d = t1 - t0;
        return d.count();
    }

    std::vector<double> timeCallMulti(const std::function<void()>& f, int runs) {
        std::vector<double> res;
        res.reserve(static_cast<std::size_t>(runs));
        for (int i = 0; i < runs; ++i) {
            res.push_back(timeCall(f));
        }
        return res;
    }

    double median(std::vector<double> v) {
        if (v.empty()) return 0.0;
        std::sort(v.begin(), v.end());
        size_t m = v.size() / 2;
        if (v.size() % 2 == 1) {
            return v[m];
        }
        return 0.5 * (v[m-1] + v[m]);
    }

    double stddev(const std::vector<double>& v) {
        if (v.size() < 2) return 0.0;
        double mean = 0.0;
        for (double x : v) {
            mean += x;
        }
        mean /= static_cast<double>(v.size());
        
        double s = 0.0;
        for (double x : v) {
            s += (x - mean) * (x - mean);
        }
        return std::sqrt(s / static_cast<double>(v.size() - 1));
    }
}
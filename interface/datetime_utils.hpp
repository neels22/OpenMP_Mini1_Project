#pragma once

#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>

/**
 * @file datetime_utils.hpp
 * @brief Date/time parsing and formatting utilities
 * 
 * Provides utilities for parsing ISO 8601 datetime strings and converting
 * to Unix timestamps. No external dependencies - implements parsing manually.
 */

namespace DateTimeUtils {

/**
 * @brief Parse ISO 8601 datetime string to Unix timestamp
 * @param dateTimeStr ISO 8601 formatted string (e.g., "2020-08-10T01:00")
 * @return Unix timestamp (seconds since epoch), or 0 if parsing fails
 * 
 * Supports formats:
 * - YYYY-MM-DDTHH:MM
 * - YYYY-MM-DDTHH:MM:SS
 * - YYYY-MM-DD HH:MM:SS
 * 
 * Assumes UTC timezone if not specified.
 */
inline long long parseISO8601(const std::string& dateTimeStr) {
    if (dateTimeStr.empty()) return 0;
    
    struct tm timeinfo = {};
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    
    // Try parsing YYYY-MM-DDTHH:MM:SS
    int parsed = sscanf(dateTimeStr.c_str(), "%d-%d-%dT%d:%d:%d",
                       &year, &month, &day, &hour, &minute, &second);
    
    if (parsed < 5) {
        // Try parsing YYYY-MM-DD HH:MM:SS
        parsed = sscanf(dateTimeStr.c_str(), "%d-%d-%d %d:%d:%d",
                       &year, &month, &day, &hour, &minute, &second);
    }
    
    if (parsed < 5) {
        // Parsing failed
        return 0;
    }
    
    // Fill tm structure
    timeinfo.tm_year = year - 1900;  // Years since 1900
    timeinfo.tm_mon = month - 1;     // Months since January (0-11)
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;
    timeinfo.tm_isdst = 0;           // Not daylight saving time
    
    // Validate ranges
    if (timeinfo.tm_year < 100 || timeinfo.tm_year > 200) return 0;  // 2000-2100
    if (timeinfo.tm_mon < 0 || timeinfo.tm_mon > 11) return 0;
    if (timeinfo.tm_mday < 1 || timeinfo.tm_mday > 31) return 0;
    if (timeinfo.tm_hour < 0 || timeinfo.tm_hour > 23) return 0;
    if (timeinfo.tm_min < 0 || timeinfo.tm_min > 59) return 0;
    if (timeinfo.tm_sec < 0 || timeinfo.tm_sec > 59) return 0;
    
    // Convert to timestamp (assuming UTC)
    #ifdef _WIN32
        return static_cast<long long>(_mkgmtime(&timeinfo));
    #else
        return static_cast<long long>(timegm(&timeinfo));
    #endif
}

/**
 * @brief Format Unix timestamp to ISO 8601 string
 * @param timestamp Unix timestamp (seconds since epoch)
 * @return Formatted datetime string (YYYY-MM-DD HH:MM:SS)
 * 
 * Converts timestamp back to human-readable format for display.
 */
inline std::string formatTimestamp(long long timestamp) {
    if (timestamp <= 0) return "Invalid";
    
    time_t t = static_cast<time_t>(timestamp);
    struct tm timeinfo;
    
    #ifdef _WIN32
        gmtime_s(&timeinfo, &t);
    #else
        gmtime_r(&t, &timeinfo);
    #endif
    
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << (timeinfo.tm_year + 1900) << "-"
        << std::setw(2) << (timeinfo.tm_mon + 1) << "-"
        << std::setw(2) << timeinfo.tm_mday << " "
        << std::setw(2) << timeinfo.tm_hour << ":"
        << std::setw(2) << timeinfo.tm_min << ":"
        << std::setw(2) << timeinfo.tm_sec;
    
    return oss.str();
}

/**
 * @brief Extract hour from Unix timestamp
 * @param timestamp Unix timestamp
 * @return Hour (0-23)
 */
inline int getHour(long long timestamp) {
    if (timestamp <= 0) return 0;
    time_t t = static_cast<time_t>(timestamp);
    struct tm timeinfo;
    #ifdef _WIN32
        gmtime_s(&timeinfo, &t);
    #else
        gmtime_r(&t, &timeinfo);
    #endif
    return timeinfo.tm_hour;
}

/**
 * @brief Extract day from Unix timestamp
 * @param timestamp Unix timestamp
 * @return Day of month (1-31)
 */
inline int getDay(long long timestamp) {
    if (timestamp <= 0) return 0;
    time_t t = static_cast<time_t>(timestamp);
    struct tm timeinfo;
    #ifdef _WIN32
        gmtime_s(&timeinfo, &t);
    #else
        gmtime_r(&t, &timeinfo);
    #endif
    return timeinfo.tm_mday;
}

/**
 * @brief Round timestamp to nearest hour
 * @param timestamp Unix timestamp
 * @return Timestamp rounded to hour boundary
 */
inline long long roundToHour(long long timestamp) {
    // Round to nearest hour (3600 seconds)
    return (timestamp / 3600) * 3600;
}

/**
 * @brief Add hours to timestamp
 * @param timestamp Base timestamp
 * @param hours Number of hours to add (can be negative)
 * @return New timestamp
 */
inline long long addHours(long long timestamp, int hours) {
    return timestamp + (hours * 3600);
}

/**
 * @brief Check if timestamp is valid
 * @param timestamp Unix timestamp
 * @return true if timestamp is in reasonable range
 */
inline bool isValidTimestamp(long long timestamp) {
    // Valid range: 2000-01-01 to 2100-01-01
    return timestamp >= 946684800 && timestamp <= 4102444800;
}

} // namespace DateTimeUtils


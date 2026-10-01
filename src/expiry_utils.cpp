#include "expiry_utils.h"
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <iostream>

std::string ExpiryUtils::getCurrentDate() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm ltm{};
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&ltm, &now_time);
#else
    localtime_r(&now_time, &ltm);
#endif

    std::ostringstream oss;
    oss << std::setfill('0')
        << (ltm.tm_year + 1900) << "-"
        << std::setw(2) << (ltm.tm_mon + 1) << "-"
        << std::setw(2) << ltm.tm_mday;
    return oss.str();
}

static bool parseDateToTm(const std::string& dateStr, std::tm& outTm) {
    if (dateStr.length() != 10) return false;
    try {
        int year = std::stoi(dateStr.substr(0, 4));
        int month = std::stoi(dateStr.substr(5, 2));
        int day = std::stoi(dateStr.substr(8, 2));

        outTm = std::tm{};
        outTm.tm_year = year - 1900;
        outTm.tm_mon = month - 1;
        outTm.tm_mday = day;
        outTm.tm_hour = 12; // Midday to prevent DST edge shifts
        outTm.tm_min = 0;
        outTm.tm_sec = 0;
        outTm.tm_isdst = -1;
        return true;
    } catch (...) {
        return false;
    }
}

int ExpiryUtils::calculateDaysUntilExpiry(const std::string& expiryDate, const std::string& referenceDate) {
    std::string refDate = referenceDate.empty() ? getCurrentDate() : referenceDate;

    std::tm expTm{};
    std::tm refTm{};

    if (!parseDateToTm(expiryDate, expTm) || !parseDateToTm(refDate, refTm)) {
        return 0;
    }

    std::time_t expTime = std::mktime(&expTm);
    std::time_t refTime = std::mktime(&refTm);

    if (expTime == static_cast<std::time_t>(-1) || refTime == static_cast<std::time_t>(-1)) {
        return 0;
    }

    double diffSeconds = std::difftime(expTime, refTime);
    int days = static_cast<int>(std::round(diffSeconds / 86400.0));
    return days;
}

ExpiryStatus ExpiryUtils::getExpiryStatus(int daysUntilExpiry, const ExpiryThresholds& thresholds) {
    if (daysUntilExpiry < 0) {
        return ExpiryStatus::EXPIRED;
    } else if (daysUntilExpiry <= thresholds.criticalDays) {
        return ExpiryStatus::CRITICAL;
    } else if (daysUntilExpiry <= thresholds.warningDays) {
        return ExpiryStatus::WARNING;
    } else {
        return ExpiryStatus::NORMAL;
    }
}

std::string ExpiryUtils::expiryStatusToString(ExpiryStatus status) {
    switch (status) {
        case ExpiryStatus::EXPIRED:  return "EXPIRED";
        case ExpiryStatus::CRITICAL: return "CRITICAL";
        case ExpiryStatus::WARNING:  return "WARNING";
        case ExpiryStatus::NORMAL:   return "NORMAL";
        default:                     return "UNKNOWN";
    }
}

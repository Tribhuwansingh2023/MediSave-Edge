#ifndef EXPIRY_UTILS_H
#define EXPIRY_UTILS_H

#include <string>

/**
 * @enum ExpiryStatus
 * @brief Categorization of medicine expiration urgency.
 */
enum class ExpiryStatus {
    EXPIRED,   // Past expiry date (days < 0)
    CRITICAL,  // Expiring within critical threshold (default: 0-7 days)
    WARNING,   // Expiring within warning threshold (default: 8-30 days)
    NORMAL     // Safe shelf-life (> 30 days)
};

/**
 * @struct ExpiryThresholds
 * @brief Configurable warning boundaries for expiry triage.
 */
struct ExpiryThresholds {
    int criticalDays = 7;
    int warningDays = 30;
};

/**
 * @class ExpiryUtils
 * @brief Date calculation and expiry evaluation helper methods.
 */
class ExpiryUtils {
public:
    // Retrieves current system date as "YYYY-MM-DD"
    static std::string getCurrentDate();

    // Calculates days remaining until expiryDate relative to referenceDate (defaults to today)
    // Returns negative value if already expired.
    static int calculateDaysUntilExpiry(const std::string& expiryDate, const std::string& referenceDate = "");

    // Evaluates ExpiryStatus from remaining days
    static ExpiryStatus getExpiryStatus(int daysUntilExpiry, const ExpiryThresholds& thresholds = ExpiryThresholds());

    // Converts ExpiryStatus enum to human-readable string
    static std::string expiryStatusToString(ExpiryStatus status);
};

#endif // EXPIRY_UTILS_H

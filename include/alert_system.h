#ifndef ALERT_SYSTEM_H
#define ALERT_SYSTEM_H

#include "inventory_manager.h"
#include "expiry_utils.h"
#include "StorageMonitor.h"
#include <queue>
#include <vector>
#include <string>

/**
 * @struct Alert
 * @brief Represents an individual triage alert with priority scoring.
 */
struct Alert {
    std::string severity;       // "[CRITICAL]", "[WARNING]", "[INFO]"
    std::string medicineName;
    std::string batchNumber;
    std::string statusMessage;
    int urgencyScore;           // Higher score = higher priority in max-heap
};

/**
 * @struct AlertComparator
 * @brief Functor to order Alerts in an STL std::priority_queue (Max-Heap).
 */
struct AlertComparator {
    bool operator()(const Alert& a, const Alert& b) const {
        return a.urgencyScore < b.urgencyScore;
    }
};

/**
 * @class AlertSystem
 * @brief Evaluates inventory state and storage conditions, dispatching priority alerts.
 */
class AlertSystem {
public:
    using AlertPriorityQueue = std::priority_queue<Alert, std::vector<Alert>, AlertComparator>;

    // Generates a max-heap of all active inventory and storage condition alerts
    static AlertPriorityQueue generateAlerts(const InventoryManager& inv,
                                             const ExpiryThresholds& thresholds = ExpiryThresholds(),
                                             const std::string& refDate = "",
                                             StorageMonitor* storageMonitor = nullptr);

    // Prints formatted CLI alert dashboard
    static void displayAlerts(const InventoryManager& inv,
                              const ExpiryThresholds& thresholds = ExpiryThresholds(),
                              const std::string& refDate = "",
                              StorageMonitor* storageMonitor = nullptr);
};

#endif // ALERT_SYSTEM_H

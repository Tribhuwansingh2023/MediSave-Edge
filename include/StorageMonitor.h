#ifndef STORAGE_MONITOR_H
#define STORAGE_MONITOR_H

#include "DeviceSensor.h"
#include <string>

/**
 * @class StorageMonitor
 * @brief High-level monitoring component bridging DeviceSensor and application alert logic.
 *
 * Queries environmental storage conditions from the kernel character device driver,
 * evaluates chamber state, formats terminal views, and generates alert messages.
 */
class StorageMonitor {
private:
    DeviceSensor& sensor;

public:
    explicit StorageMonitor(DeviceSensor& sensor);

    // Queries current live condition from device driver
    bool getCurrentCondition(double& temperature, std::string& status);

    // Formatted terminal display
    void displayCondition();

    // Condition evaluations
    bool isCritical();
    bool isWarning();
    bool isAvailable();

    // Summary alert string for dashboard integration
    std::string getConditionAlert();
};

#endif // STORAGE_MONITOR_H

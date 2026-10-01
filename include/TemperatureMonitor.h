#ifndef TEMPERATURE_MONITOR_H
#define TEMPERATURE_MONITOR_H

#include "DeviceSensor.h"
#include <string>
#include <vector>
#include <mutex>

/**
 * @struct TemperatureReading
 * @brief Telemetry snapshot from the Linux character driver.
 */
struct TemperatureReading {
    double temperature{6.50};
    std::string status{"NORMAL"};
    std::string timestamp;
    bool valid{false};
};

/**
 * @class TemperatureMonitor
 * @brief Centralized storage monitoring component communicating with DeviceSensor
 *        and feeding the alert engine.
 *
 * Responsibilities:
 * - communicate with DeviceSensor (/dev/medisave)
 * - maintain latest temperature
 * - maintain latest status (LOW, NORMAL, WARNING, CRITICAL)
 * - track monitoring timestamp
 * - generate storage alerts
 * - expose thread-safe current state and reading history
 */
class TemperatureMonitor {
private:
    DeviceSensor& sensor;
    mutable std::mutex monitorMutex;
    TemperatureReading latestReading;
    std::vector<TemperatureReading> history;
    size_t maxHistorySize{50};

    static std::string getCurrentTimestampString();

public:
    explicit TemperatureMonitor(DeviceSensor& s);

    // Queries live device driver and updates internal state
    bool updateReading();

    // Query thread-safe state
    TemperatureReading getLatestReading() const;
    bool getLastCondition(double& temperature, std::string& status, std::string& timestamp) const;

    // Simulation / testing control
    bool setSimulatedTemperature(double temp);

    // Storage alert formatting
    std::string getStorageAlert() const;

    // Formatted terminal display
    void displayCondition() const;

    // Status queries
    bool isCritical() const;
    bool isWarning() const;
    bool isAvailable();
    std::string getDevicePath() const;

    // History
    std::vector<TemperatureReading> getHistory() const;
    void clearHistory();

    // Threshold classification matching kernel driver
    static std::string classifyTemperature(double temp);
};

#endif // TEMPERATURE_MONITOR_H

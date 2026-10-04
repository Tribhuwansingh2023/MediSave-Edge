#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <string>

/**
 * @struct CpuInfo
 * @brief Telemetry extracted from /proc/cpuinfo and /proc/stat.
 */
struct CpuInfo {
    std::string modelName{"Unknown CPU"};
    int logicalCores{1};
    double usagePercent{0.0};
};

/**
 * @struct MemoryInfo
 * @brief Telemetry extracted from /proc/meminfo.
 */
struct MemoryInfo {
    double totalGB{0.0};
    double usedGB{0.0};
    double availableGB{0.0};
    double usagePercent{0.0};
};

/**
 * @struct ServiceStatus
 * @brief Current operational status of core MediSave Edge services.
 */
struct ServiceStatus {
    std::string driver{"UNAVAILABLE"};
    std::string storageSensor{"UNAVAILABLE"};
    std::string tcpServer{"STOPPED"};
    std::string monitoring{"STOPPED"};
};

/**
 * @struct DashboardSnapshot
 * @brief Aggregated operational status for the MediSave Edge executive dashboard.
 */
struct DashboardSnapshot {
    // Storage
    double temperature{6.50};
    bool temperatureValid{false};
    std::string storageStatus{"NORMAL"};

    // Inventory
    int totalMedicines{0};
    int lowStockCount{0};
    int expiringCount{0};
    int expiredCount{0};

    // Redistribution
    int facilityCount{0};
    int shortageCount{0};
    int surplusCount{0};
    int recommendationCount{0};

    // System
    double cpuUsagePercent{0.0};
    double memoryUsagePercent{0.0};
    std::string uptimeFormatted{"0m"};

    // Services
    std::string driverStatus{"UNAVAILABLE"};
    std::string monitoringStatus{"STOPPED"};
    std::string tcpStatus{"STOPPED"};
};

/**
 * @class SystemMonitor
 * @brief Low-level Linux system monitoring reading the /proc virtual filesystem.
 *
 * Direct VFS parsers:
 * - /proc/cpuinfo (CPU model, core counts)
 * - /proc/stat    (Multi-sample delta CPU utilization)
 * - /proc/meminfo (MemTotal, MemAvailable, memory utilization)
 * - /proc/uptime  (Kernel system uptime)
 *
 * Includes graceful fallback and mock /proc path support for cross-platform validation.
 */
class SystemMonitor {
private:
    std::string procPathPrefix{""}; // Defaults to "" (accessing /proc/...)

public:
    SystemMonitor() = default;
    explicit SystemMonitor(const std::string& prefix);

    // /proc path configuration (useful for deterministic unit tests)
    void setProcPrefix(const std::string& prefix);
    std::string getProcPrefix() const;

    // CPU Metrics (/proc/cpuinfo and /proc/stat)
    bool getCpuInfo(CpuInfo& outCpu, int sampleIntervalMs = 100) const;
    double sampleCpuUsage(int sampleIntervalMs = 100) const;

    // Memory Metrics (/proc/meminfo)
    bool getMemoryInfo(MemoryInfo& outMem) const;

    // Uptime Metrics (/proc/uptime)
    bool getUptime(double& uptimeSeconds, std::string& formattedUptime) const;

    // Formatted Health and Dashboard displays
    void displaySystemHealth(const ServiceStatus& services) const;
    static void displaySystemDashboard(const DashboardSnapshot& data);

    // Static formatters
    static std::string formatUptimeString(double seconds);
};

#endif // SYSTEM_MONITOR_H

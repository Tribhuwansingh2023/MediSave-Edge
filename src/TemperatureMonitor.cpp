#include "TemperatureMonitor.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>

TemperatureMonitor::TemperatureMonitor(DeviceSensor& s)
    : sensor(s) {
    latestReading.temperature = 6.50;
    latestReading.status = "NORMAL";
    latestReading.timestamp = getCurrentTimestampString();
    latestReading.valid = false;
}

std::string TemperatureMonitor::getCurrentTimestampString() {
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm tmSnapshot{};
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tmSnapshot, &nowTime);
#else
    localtime_r(&nowTime, &tmSnapshot);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmSnapshot);
    return std::string(buf);
}

std::string TemperatureMonitor::classifyTemperature(double temp) {
    if (temp < 2.0) {
        return "LOW";
    } else if (temp <= 8.0) {
        return "NORMAL";
    } else if (temp <= 10.0) {
        return "WARNING";
    } else {
        return "CRITICAL";
    }
}

bool TemperatureMonitor::updateReading() {
    std::lock_guard<std::mutex> lock(monitorMutex);

    if (!sensor.isConnected()) {
        if (!sensor.connect()) {
            latestReading.status = "UNAVAILABLE";
            latestReading.valid = false;
            latestReading.timestamp = getCurrentTimestampString();
            return false;
        }
    }

    double temp = 0.0;
    std::string stat;
    bool tempOk = sensor.readTemperature(temp);
    bool statOk = sensor.getStatus(stat);

    latestReading.timestamp = getCurrentTimestampString();

    if (tempOk && statOk) {
        latestReading.temperature = temp;
        latestReading.status = stat;
        latestReading.valid = true;

        history.push_back(latestReading);
        if (history.size() > maxHistorySize) {
            history.erase(history.begin());
        }
        return true;
    } else {
        latestReading.status = "UNAVAILABLE";
        latestReading.valid = false;
        return false;
    }
}

TemperatureReading TemperatureMonitor::getLatestReading() const {
    std::lock_guard<std::mutex> lock(monitorMutex);
    return latestReading;
}

bool TemperatureMonitor::getLastCondition(double& temperature, std::string& status, std::string& timestamp) const {
    std::lock_guard<std::mutex> lock(monitorMutex);
    temperature = latestReading.temperature;
    status = latestReading.status;
    timestamp = latestReading.timestamp;
    return latestReading.valid;
}

bool TemperatureMonitor::setSimulatedTemperature(double temp) {
    if (!sensor.setTemperature(temp)) {
        return false;
    }
    return updateReading();
}

std::string TemperatureMonitor::getStorageAlert() const {
    std::lock_guard<std::mutex> lock(monitorMutex);
    std::ostringstream oss;

    if (!latestReading.valid) {
        return "[STORAGE UNAVAILABLE]\nSensor driver not detected at " + sensor.getDevicePath();
    }

    oss << "[STORAGE " << latestReading.status << "]\n";
    oss << "Temperature: " << std::fixed << std::setprecision(2) << latestReading.temperature << " C\n";

    if (latestReading.status == "CRITICAL") {
        oss << "Storage condition requires immediate attention.";
    } else if (latestReading.status == "WARNING" || latestReading.status == "LOW") {
        oss << "Storage condition requires attention.";
    }

    return oss.str();
}

void TemperatureMonitor::displayCondition() const {
    std::lock_guard<std::mutex> lock(monitorMutex);

    if (!latestReading.valid) {
        std::cout << "\n----------------------------------------\n";
        std::cout << "STORAGE SENSOR ERROR\n";
        std::cout << "----------------------------------------\n";
        std::cout << sensor.getDevicePath() << " is unavailable.\n\n";
        std::cout << "Please load the MediSave Edge Linux\n";
        std::cout << "device driver before using storage\n";
        std::cout << "monitoring.\n\n";
        std::cout << "Inventory features remain available.\n";
        std::cout << "----------------------------------------\n";
        return;
    }

    std::cout << "\n========================================\n";
    std::cout << "       STORAGE CONDITION\n";
    std::cout << "========================================\n\n";
    std::cout << "Temperature : " << std::fixed << std::setprecision(2) << latestReading.temperature << " C\n";
    std::cout << "Status      : " << latestReading.status << "\n";
    std::cout << "Timestamp   : " << latestReading.timestamp << "\n\n";

    if (latestReading.status == "CRITICAL") {
        std::cout << "CRITICAL: Storage condition requires immediate attention.\n\n";
    } else if (latestReading.status == "WARNING" || latestReading.status == "LOW") {
        std::cout << "WARNING: Storage condition requires attention.\n\n";
    }

    std::cout << "========================================\n";
}

bool TemperatureMonitor::isCritical() const {
    std::lock_guard<std::mutex> lock(monitorMutex);
    return latestReading.valid && (latestReading.status == "CRITICAL");
}

bool TemperatureMonitor::isWarning() const {
    std::lock_guard<std::mutex> lock(monitorMutex);
    return latestReading.valid && (latestReading.status == "WARNING" || latestReading.status == "LOW");
}

bool TemperatureMonitor::isAvailable() {
    if (!sensor.isConnected()) {
        return sensor.connect();
    }
    return true;
}

std::string TemperatureMonitor::getDevicePath() const {
    return sensor.getDevicePath();
}

std::vector<TemperatureReading> TemperatureMonitor::getHistory() const {
    std::lock_guard<std::mutex> lock(monitorMutex);
    return history;
}

void TemperatureMonitor::clearHistory() {
    std::lock_guard<std::mutex> lock(monitorMutex);
    history.clear();
}

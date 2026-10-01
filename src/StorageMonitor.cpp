#include "StorageMonitor.h"
#include <iostream>
#include <iomanip>
#include <sstream>

StorageMonitor::StorageMonitor(DeviceSensor& s) : sensor(s) {}

bool StorageMonitor::getCurrentCondition(double& temperature, std::string& status) {
    if (!sensor.isConnected()) {
        if (!sensor.connect()) {
            return false;
        }
    }
    bool tempOk = sensor.readTemperature(temperature);
    bool statOk = sensor.getStatus(status);
    return (tempOk && statOk);
}

bool StorageMonitor::isAvailable() {
    if (!sensor.isConnected()) {
        return sensor.connect();
    }
    return true;
}

bool StorageMonitor::isCritical() {
    double temp;
    std::string status;
    if (getCurrentCondition(temp, status)) {
        return (status == "CRITICAL");
    }
    return false;
}

bool StorageMonitor::isWarning() {
    double temp;
    std::string status;
    if (getCurrentCondition(temp, status)) {
        return (status == "WARNING" || status == "LOW");
    }
    return false;
}

void StorageMonitor::displayCondition() {
    double temp = 0.0;
    std::string status;

    if (!getCurrentCondition(temp, status)) {
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
    std::cout << "Temperature : " << std::fixed << std::setprecision(2) << temp << " C\n";
    std::cout << "Status      : " << status << "\n\n";

    if (status == "CRITICAL" || status == "WARNING" || status == "LOW") {
        std::cout << "WARNING: Storage condition requires attention.\n\n";
    }

    std::cout << "========================================\n";
}

std::string StorageMonitor::getConditionAlert() {
    double temp;
    std::string status;
    if (!getCurrentCondition(temp, status)) {
        return "[STORAGE UNAVAILABLE] Sensor driver not detected at " + sensor.getDevicePath();
    }

    std::ostringstream oss;
    oss << "[STORAGE " << status << "] Temperature: "
        << std::fixed << std::setprecision(2) << temp << " C";
    if (status != "NORMAL") {
        oss << " - Storage condition requires attention.";
    }
    return oss.str();
}

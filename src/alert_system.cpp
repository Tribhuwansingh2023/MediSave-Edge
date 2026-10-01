#include "alert_system.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>

AlertSystem::AlertPriorityQueue AlertSystem::generateAlerts(const InventoryManager& inv,
                                                            const ExpiryThresholds& thresholds,
                                                            const std::string& refDate,
                                                            StorageMonitor* storageMonitor) {
    AlertPriorityQueue queue;
    std::vector<Medicine> meds = inv.getAllMedicines();

    // 1. Environmental Storage Condition Analysis (Kernel Driver)
    if (storageMonitor != nullptr) {
        double temp = 0.0;
        std::string status;
        if (storageMonitor->getCurrentCondition(temp, status)) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << temp;
            if (status == "CRITICAL") {
                Alert alert;
                alert.severity = "[CRITICAL]";
                alert.medicineName = "Storage Chamber (/dev/medisave)";
                alert.batchNumber = "CHAMBER-01";
                alert.statusMessage = "STORAGE CRITICAL (" + oss.str() + " C) - Storage condition requires attention.";
                alert.urgencyScore = 950;
                queue.push(alert);
            } else if (status == "WARNING" || status == "LOW") {
                Alert alert;
                alert.severity = "[WARNING]";
                alert.medicineName = "Storage Chamber (/dev/medisave)";
                alert.batchNumber = "CHAMBER-01";
                alert.statusMessage = "STORAGE " + status + " (" + oss.str() + " C) - Storage condition requires attention.";
                alert.urgencyScore = 320;
                queue.push(alert);
            }
        }
    }

    // 2. Medicine Inventory & Expiry Analysis
    for (const auto& med : meds) {
        int days = ExpiryUtils::calculateDaysUntilExpiry(med.getExpiryDate(), refDate);

        // Expiry analysis
        if (days < 0) {
            Alert alert;
            alert.severity = "[CRITICAL]";
            alert.medicineName = med.getName();
            alert.batchNumber = med.getBatchNumber();
            alert.statusMessage = "EXPIRED " + std::to_string(std::abs(days)) + " DAYS AGO";
            alert.urgencyScore = 1000 + std::abs(days);
            queue.push(alert);
        } else if (days <= thresholds.criticalDays) {
            Alert alert;
            alert.severity = "[CRITICAL]";
            alert.medicineName = med.getName();
            alert.batchNumber = med.getBatchNumber();
            if (days == 0) {
                alert.statusMessage = "EXPIRES TODAY";
            } else {
                alert.statusMessage = "EXPIRES IN " + std::to_string(days) + (days == 1 ? " DAY" : " DAYS");
            }
            alert.urgencyScore = 500 + (thresholds.criticalDays - days) * 50;
            queue.push(alert);
        } else if (days <= thresholds.warningDays) {
            Alert alert;
            alert.severity = "[WARNING]";
            alert.medicineName = med.getName();
            alert.batchNumber = med.getBatchNumber();
            alert.statusMessage = "EXPIRES IN " + std::to_string(days) + " DAYS";
            alert.urgencyScore = 100 + (thresholds.warningDays - days);
            queue.push(alert);
        }

        // Stock level analysis
        if (med.isOutOfStock()) {
            Alert alert;
            alert.severity = "[CRITICAL]";
            alert.medicineName = med.getName();
            alert.batchNumber = med.getBatchNumber();
            alert.statusMessage = "OUT OF STOCK (0 units)";
            alert.urgencyScore = 400;
            queue.push(alert);
        } else if (med.isLowStock()) {
            Alert alert;
            alert.severity = "[WARNING]";
            alert.medicineName = med.getName();
            alert.batchNumber = med.getBatchNumber();
            alert.statusMessage = "LOW STOCK (" + std::to_string(med.getQuantity()) + " / Min: " + std::to_string(med.getMinStock()) + ")";
            alert.urgencyScore = 250 + (med.getMinStock() - med.getQuantity()) * 5;
            queue.push(alert);
        }
    }

    return queue;
}

void AlertSystem::displayAlerts(const InventoryManager& inv,
                                const ExpiryThresholds& thresholds,
                                const std::string& refDate,
                                StorageMonitor* storageMonitor) {
    AlertPriorityQueue queue = generateAlerts(inv, thresholds, refDate, storageMonitor);

    std::cout << "\n========================================\n";
    std::cout << "        MEDISAVE EDGE ALERTS\n";
    std::cout << "========================================\n";

    if (storageMonitor != nullptr) {
        double temp = 0.0;
        std::string status;
        if (storageMonitor->getCurrentCondition(temp, status)) {
            std::cout << "\n[STORAGE " << status << "] Temperature: "
                      << std::fixed << std::setprecision(2) << temp << " C";
            if (status != "NORMAL") {
                std::cout << " - Storage condition requires attention.";
            }
            std::cout << "\n";
        } else {
            std::cout << "\n[STORAGE NOTICE] Sensor driver not detected at /dev/medisave.\n";
        }
    }

    if (queue.empty()) {
        std::cout << "\n [INFO] All medicine stock levels and expiry dates\n";
        std::cout << "        are currently within normal parameters.\n";
    } else {
        while (!queue.empty()) {
            Alert alert = queue.top();
            queue.pop();

            std::cout << "\n" << alert.severity << " " << alert.medicineName << "\n";
            std::cout << "Batch: " << alert.batchNumber << "\n";
            std::cout << "Status: " << alert.statusMessage << "\n";
        }
    }

    std::cout << "\n========================================\n";
}

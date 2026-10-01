#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <limits>
#include <csignal>

#include "medicine.h"
#include "inventory_manager.h"
#include "expiry_utils.h"
#include "alert_system.h"
#include "DeviceSensor.h"
#include "StorageMonitor.h"
#include "ProcessManager.h"

#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#endif

// Default persistent data file path
static const std::string DATA_FILE_PATH = "data/medicines.txt";

// Async-signal-safe flags
static volatile sig_atomic_t g_shutdownRequested = 0;
static volatile sig_atomic_t g_usr1Requested = 0;

static void masterSignalHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        g_shutdownRequested = 1;
    }
#if defined(SIGUSR1)
    else if (signum == SIGUSR1) {
        g_usr1Requested = 1;
    }
#endif
}

// Safe input helper functions
static void clearCin() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

static std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string val;
    std::getline(std::cin, val);
    return val;
}

static std::string readNonEmptyString(const std::string& prompt) {
    while (!g_shutdownRequested) {
        std::string s = readLine(prompt);
        if (!s.empty()) {
            return s;
        }
        std::cout << " [Error] Input cannot be empty. Please try again.\n";
    }
    return "";
}

static int readInt(const std::string& prompt, int minVal = 0, int maxVal = 10000000) {
    while (!g_shutdownRequested) {
        std::cout << prompt;
        int val;
        if (std::cin >> val) {
            if (val >= minVal && val <= maxVal) {
                clearCin();
                return val;
            }
            std::cout << " [Error] Value must be between " << minVal << " and " << maxVal << ".\n";
        } else {
            if (g_shutdownRequested) return 16;
            std::cout << " [Error] Invalid integer input. Please try again.\n";
        }
        clearCin();
    }
    return 16;
}

static double readDouble(const std::string& prompt, double minVal = -50.0, double maxVal = 100.0) {
    while (!g_shutdownRequested) {
        std::cout << prompt;
        double val;
        if (std::cin >> val) {
            if (val >= minVal && val <= maxVal) {
                clearCin();
                return val;
            }
            std::cout << " [Error] Value must be between " << minVal << " and " << maxVal << ".\n";
        } else {
            if (g_shutdownRequested) return 0.0;
            std::cout << " [Error] Invalid numeric input. Please try again.\n";
        }
        clearCin();
    }
    return 0.0;
}

static std::string readDate(const std::string& prompt) {
    while (!g_shutdownRequested) {
        std::string date = readNonEmptyString(prompt);
        if (Medicine::isValidDate(date)) {
            return date;
        }
        std::cout << " [Error] Invalid date format! Expected YYYY-MM-DD (e.g., 2026-12-31).\n";
    }
    return "2099-12-31";
}

// Formatted display helper
static void printMedicineRow(const Medicine& med) {
    int days = ExpiryUtils::calculateDaysUntilExpiry(med.getExpiryDate());
    ExpiryStatus status = ExpiryUtils::getExpiryStatus(days);
    std::string statusStr = ExpiryUtils::expiryStatusToString(status);

    std::cout << std::left << std::setw(10) << med.getId()
              << std::setw(20) << (med.getName().length() > 18 ? med.getName().substr(0, 15) + "..." : med.getName())
              << std::setw(12) << med.getBatchNumber()
              << std::right << std::setw(6) << med.getQuantity() << "  "
              << std::left << std::setw(12) << med.getExpiryDate()
              << std::setw(10) << statusStr
              << std::right << std::setw(5) << med.getMinStock() << "/" << std::left << std::setw(5) << med.getMaxStock()
              << " [" << std::fixed << std::setprecision(1) << med.getMinTemperature()
              << "C to " << med.getMaxTemperature() << "C]\n";
}

static void displayInventoryTable(const std::vector<Medicine>& meds) {
    if (meds.empty()) {
        std::cout << "\n [Notice] Inventory is currently empty.\n";
        return;
    }

    std::cout << "\n" << std::string(88, '-') << "\n";
    std::cout << std::left << std::setw(10) << "ID"
              << std::setw(20) << "Medicine Name"
              << std::setw(12) << "Batch"
              << std::right << std::setw(8) << "Stock" << "  "
              << std::left << std::setw(12) << "Expiry Date"
              << std::setw(10) << "Status"
              << std::setw(12) << "Min/Max"
              << "Temp Range\n";
    std::cout << std::string(88, '-') << "\n";

    int totalStock = 0;
    for (const auto& med : meds) {
        printMedicineRow(med);
        totalStock += med.getQuantity();
    }
    std::cout << std::string(88, '-') << "\n";
    std::cout << "Total distinct medicine records: " << meds.size()
              << " | Total cumulative stock units: " << totalStock << "\n";
}

// Menu actions
static void handleAddMedicine(InventoryManager& inv) {
    std::cout << "\n--- Add New Medicine ---\n";
    std::string id = readNonEmptyString("Enter Medicine ID (e.g. MED-001): ");
    if (!Medicine::isValidId(id)) {
        std::cout << " [Error] ID must contain only alphanumeric characters, '-', or '_'.\n";
        return;
    }
    if (inv.findMedicineById(id) != nullptr) {
        std::cout << " [Error] Medicine ID '" << id << "' already exists in inventory!\n";
        return;
    }

    std::string name = readNonEmptyString("Enter Medicine Name: ");
    std::string batch = readNonEmptyString("Enter Batch Number: ");
    int qty = readInt("Enter Initial Quantity: ", 0, 1000000);
    std::string expiry = readDate("Enter Expiry Date (YYYY-MM-DD): ");
    int minStock = readInt("Enter Minimum Required Stock: ", 0, 1000000);
    int maxStock = readInt("Enter Maximum Stock Capacity: ", minStock, 1000000);
    double minTemp = readDouble("Enter Minimum Storage Temperature (°C): ", -30.0, 50.0);
    double maxTemp = readDouble("Enter Maximum Storage Temperature (°C): ", minTemp, 50.0);

    try {
        Medicine med(id, name, batch, qty, expiry, minStock, maxStock, minTemp, maxTemp);
        if (inv.addMedicine(med)) {
            std::cout << " [Success] Medicine '" << name << "' added successfully.\n";
            inv.saveToFile(DATA_FILE_PATH);
        } else {
            std::cout << " [Error] Failed to add medicine.\n";
        }
    } catch (const std::exception& ex) {
        std::cout << " [Validation Error] " << ex.what() << "\n";
    }
}

static void handleRemoveMedicine(InventoryManager& inv) {
    std::cout << "\n--- Remove Medicine ---\n";
    std::string id = readNonEmptyString("Enter Medicine ID to remove: ");
    const Medicine* med = inv.findMedicineById(id);
    if (!med) {
        std::cout << " [Error] Medicine ID '" << id << "' not found.\n";
        return;
    }

    std::cout << "Found: " << med->getName() << " (Batch: " << med->getBatchNumber() << ", Stock: " << med->getQuantity() << ")\n";
    std::string confirm = readLine("Are you sure you want to remove this record? (y/N): ");
    if (confirm == "y" || confirm == "Y") {
        if (inv.removeMedicine(id)) {
            std::cout << " [Success] Medicine record removed.\n";
            inv.saveToFile(DATA_FILE_PATH);
        }
    } else {
        std::cout << " [Cancelled] Removal aborted.\n";
    }
}

static void handleUpdateMedicine(InventoryManager& inv) {
    std::cout << "\n--- Update Medicine Details ---\n";
    std::string id = readNonEmptyString("Enter Medicine ID to update: ");
    Medicine* med = inv.findMedicineById(id);
    if (!med) {
        std::cout << " [Error] Medicine ID '" << id << "' not found.\n";
        return;
    }

    std::cout << "\nUpdating [" << med->getId() << "] " << med->getName() << "\n";
    std::cout << "Leave blank to keep existing value.\n";

    std::string newName = readLine("New Name [" + med->getName() + "]: ");
    if (!newName.empty()) med->setName(newName);

    std::string newBatch = readLine("New Batch [" + med->getBatchNumber() + "]: ");
    if (!newBatch.empty()) med->setBatchNumber(newBatch);

    std::string dateChoice = readLine("Update Expiry Date? (y/N): ");
    if (dateChoice == "y" || dateChoice == "Y") {
        std::string newExp = readDate("New Expiry Date (YYYY-MM-DD): ");
        med->setExpiryDate(newExp);
    }

    std::string boundsChoice = readLine("Update Stock Bounds? (y/N): ");
    if (boundsChoice == "y" || boundsChoice == "Y") {
        int minS = readInt("New Min Stock: ", 0, 1000000);
        int maxS = readInt("New Max Stock: ", minS, 1000000);
        med->setStockBounds(minS, maxS);
    }

    std::string tempChoice = readLine("Update Temperature Bounds? (y/N): ");
    if (tempChoice == "y" || tempChoice == "Y") {
        double minT = readDouble("New Min Temp (°C): ", -30.0, 50.0);
        double maxT = readDouble("New Max Temp (°C): ", minT, 50.0);
        med->setTemperatureBounds(minT, maxT);
    }

    inv.saveToFile(DATA_FILE_PATH);
    std::cout << " [Success] Medicine details updated.\n";
}

static void handleSearchMedicine(const InventoryManager& inv) {
    std::cout << "\n--- Search Medicine ---\n";
    std::cout << "1. Search by Medicine ID\n";
    std::cout << "2. Search by Medicine Name\n";
    int choice = readInt("Enter choice (1-2): ", 1, 2);

    if (choice == 1) {
        std::string id = readNonEmptyString("Enter Medicine ID: ");
        const Medicine* med = inv.findMedicineById(id);
        if (med) {
            displayInventoryTable({*med});
        } else {
            std::cout << " [Notice] No medicine found with ID '" << id << "'.\n";
        }
    } else {
        std::string nameQuery = readNonEmptyString("Enter search keyword for name: ");
        std::vector<Medicine> matches = inv.findMedicinesByName(nameQuery);
        if (!matches.empty()) {
            displayInventoryTable(matches);
        } else {
            std::cout << " [Notice] No medicines matching '" << nameQuery << "' found.\n";
        }
    }
}

static void handleUpdateStock(InventoryManager& inv) {
    std::cout << "\n--- Update Stock Quantity ---\n";
    std::string id = readNonEmptyString("Enter Medicine ID: ");
    Medicine* med = inv.findMedicineById(id);
    if (!med) {
        std::cout << " [Error] Medicine ID '" << id << "' not found.\n";
        return;
    }

    std::cout << "Current stock for " << med->getName() << " [" << med->getId() << "]: "
              << med->getQuantity() << " units (Min: " << med->getMinStock()
              << ", Max: " << med->getMaxStock() << ")\n";
    std::cout << "Enter positive number to add stock, or negative number to dispense stock.\n";
    int delta = readInt("Stock adjustment delta: ", -med->getQuantity(), 1000000);

    if (inv.updateStock(id, delta)) {
        std::cout << " [Success] Stock updated. New Quantity: " << med->getQuantity() << " units.\n";
        if (med->isLowStock()) {
            std::cout << " [Warning] Stock is now at or below minimum threshold (" << med->getMinStock() << ")!\n";
        }
        inv.saveToFile(DATA_FILE_PATH);
    } else {
        std::cout << " [Error] Stock adjustment failed. Cannot reduce stock below 0.\n";
    }
}

static void handleCheckExpiry(const InventoryManager& inv) {
    std::cout << "\n========================================\n";
    std::cout << "        EXPIRY ANALYSIS REPORT\n";
    std::cout << "========================================\n";
    std::cout << "Current System Reference Date: " << ExpiryUtils::getCurrentDate() << "\n\n";

    std::vector<std::pair<Medicine, int>> expired = inv.getExpiredMedicines();
    std::vector<std::pair<Medicine, int>> expiringSoon = inv.getExpiringSoonMedicines(30);

    std::cout << "[EXPIRED MEDICINES] (Action Required: Quarantine / Dispose)\n";
    if (expired.empty()) {
        std::cout << "  No expired medicines in inventory.\n";
    } else {
        for (const auto& [med, days] : expired) {
            std::cout << "  * " << med.getName() << " [ID: " << med.getId() << ", Batch: " << med.getBatchNumber()
                      << "] Expired " << std::abs(days) << " days ago (" << med.getExpiryDate() << ") - Qty: " << med.getQuantity() << "\n";
        }
    }

    std::cout << "\n[EXPIRING SOON - WITHIN 30 DAYS] (Action Required: Prioritize Consumption / Redistribution)\n";
    if (expiringSoon.empty()) {
        std::cout << "  No medicines expiring within the next 30 days.\n";
    } else {
        for (const auto& [med, days] : expiringSoon) {
            std::string urgency = (days <= 7) ? "[CRITICAL 0-7d]" : "[WARNING 8-30d]";
            std::cout << "  * " << urgency << " " << med.getName() << " [ID: " << med.getId() << ", Batch: " << med.getBatchNumber()
                      << "] Expires in " << days << " days (" << med.getExpiryDate() << ") - Qty: " << med.getQuantity() << "\n";
        }
    }
    std::cout << "========================================\n";
}

// Storage Sensor Driver Operations
static void handleReadStorageTemperature(StorageMonitor& monitor, DeviceSensor& sensor) {
    std::cout << "\n--- Read Storage Temperature ---\n";
    double temp = 0.0;
    std::string status;

    if (monitor.getCurrentCondition(temp, status)) {
        std::cout << "----------------------------------------\n";
        std::cout << "Device Node : " << sensor.getDevicePath() << "\n";
        std::cout << "Temperature : " << std::fixed << std::setprecision(2) << temp << " C\n";
        std::cout << "Status      : " << status << "\n";
        std::cout << "----------------------------------------\n";
    } else {
        std::cout << "\n----------------------------------------\n";
        std::cout << "STORAGE SENSOR ERROR\n";
        std::cout << "----------------------------------------\n";
        std::cout << sensor.getDevicePath() << " is unavailable.\n\n";
        std::cout << "Please load the MediSave Edge Linux\n";
        std::cout << "device driver before using storage\n";
        std::cout << "monitoring.\n\n";
        std::cout << "Inventory features remain available.\n";
        std::cout << "----------------------------------------\n";
    }
}

static void handleSetSimulatedTemperature(DeviceSensor& sensor, StorageMonitor& monitor) {
    std::cout << "\n--- Set Simulated Temperature ---\n";
    if (!monitor.isAvailable()) {
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

    double newTemp = readDouble("Enter new simulated temperature (°C): ", -50.0, 100.0);
    if (sensor.setTemperature(newTemp)) {
        std::cout << " [Success] Temperature " << std::fixed << std::setprecision(2)
                  << newTemp << " C written to " << sensor.getDevicePath() << ".\n";
    } else {
        std::cout << " [Error] " << sensor.getLastError() << "\n";
    }
}

static void handleShowStorageCondition(StorageMonitor& monitor, const InventoryManager& inv) {
    monitor.displayCondition();

    double temp = 0.0;
    std::string status;
    if (monitor.getCurrentCondition(temp, status)) {
        if (status == "CRITICAL" || status == "WARNING" || status == "LOW") {
            std::vector<Medicine> breachedMeds;
            for (const auto& med : inv.getAllMedicines()) {
                if (med.isTemperatureViolated(temp)) {
                    breachedMeds.push_back(med);
                }
            }

            if (!breachedMeds.empty()) {
                std::cout << "\nThe following medicines have storage tolerances breached:\n";
                for (const auto& bm : breachedMeds) {
                    std::cout << "  * " << bm.getName() << " [ID: " << bm.getId() << "] (Allowed: "
                              << std::fixed << std::setprecision(1) << bm.getMinTemperature()
                              << "C to " << bm.getMaxTemperature() << "C) - In Stock: "
                              << bm.getQuantity() << " units\n";
                }
                std::cout << "========================================\n";
            }
        }
    }
}

int main() {
    // Register POSIX signal handling
#if defined(__linux__) || defined(__unix__)
    struct sigaction sa{};
    sa.sa_handler = masterSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGUSR1, &sa, nullptr);
#else
    std::signal(SIGINT, masterSignalHandler);
    std::signal(SIGTERM, masterSignalHandler);
#endif

    InventoryManager inventory;
    DeviceSensor sensor("/dev/medisave");
    StorageMonitor monitor(sensor);
    ProcessManager processManager;

    std::cout << "========================================\n";
    std::cout << "        MEDISAVE EDGE SYSTEM\n";
    std::cout << " Medicine Storage & Inventory Monitor\n";
    std::cout << "========================================\n";

    // Attempt to load existing inventory
    if (inventory.loadFromFile(DATA_FILE_PATH)) {
        std::cout << "[System] Loaded " << inventory.getMedicineCount()
                  << " medicine records from " << DATA_FILE_PATH << ".\n";
    } else {
        std::cout << "[System] Notice: Starting with fresh or empty inventory.\n";
        std::cout << "[System] Data will be saved to " << DATA_FILE_PATH << ".\n";
    }

    bool running = true;
    while (running && !g_shutdownRequested) {
        if (g_usr1Requested) {
            g_usr1Requested = 0;
            std::cout << "\n[Signal SIGUSR1 Detected] Polling live background monitoring snapshot...\n";
            processManager.printMonitoringStatus();
        }

        std::cout << "\n========================================\n";
        std::cout << "           MEDISAVE EDGE\n";
        std::cout << "========================================\n";
        std::cout << " 1. Add Medicine\n";
        std::cout << " 2. Remove Medicine\n";
        std::cout << " 3. Update Medicine\n";
        std::cout << " 4. Search Medicine\n";
        std::cout << " 5. Display Inventory\n";
        std::cout << " 6. Update Stock\n";
        std::cout << " 7. Check Expiry\n";
        std::cout << " 8. Show Alerts\n";
        std::cout << " 9. Save Inventory\n";
        std::cout << "10. Read Storage Temperature\n";
        std::cout << "11. Set Simulated Temperature\n";
        std::cout << "12. Show Storage Condition\n";
        std::cout << "13. Start Background Monitoring (fork/exec)\n";
        std::cout << "14. Stop Background Monitoring (SIGTERM/waitpid)\n";
        std::cout << "15. Show Monitoring Status (IPC/Pipe/Shm)\n";
        std::cout << "16. Exit\n";
        std::cout << "========================================\n";

        int choice = readInt("Enter choice: ", 1, 16);

        if (g_shutdownRequested) {
            break;
        }

        switch (choice) {
            case 1:
                handleAddMedicine(inventory);
                break;
            case 2:
                handleRemoveMedicine(inventory);
                break;
            case 3:
                handleUpdateMedicine(inventory);
                break;
            case 4:
                handleSearchMedicine(inventory);
                break;
            case 5:
                displayInventoryTable(inventory.getAllMedicines());
                break;
            case 6:
                handleUpdateStock(inventory);
                break;
            case 7:
                handleCheckExpiry(inventory);
                break;
            case 8:
                AlertSystem::displayAlerts(inventory, ExpiryThresholds(), "", &monitor);
                break;
            case 9:
                if (inventory.saveToFile(DATA_FILE_PATH)) {
                    std::cout << " [Success] Inventory saved to " << DATA_FILE_PATH << ".\n";
                } else {
                    std::cout << " [Error] Failed to write to " << DATA_FILE_PATH << ".\n";
                }
                break;
            case 10:
                handleReadStorageTemperature(monitor, sensor);
                break;
            case 11:
                handleSetSimulatedTemperature(sensor, monitor);
                break;
            case 12:
                handleShowStorageCondition(monitor, inventory);
                break;
            case 13:
                processManager.startMonitor(5);
                break;
            case 14:
                processManager.stopMonitor();
                break;
            case 15:
                processManager.printMonitoringStatus();
                break;
            case 16:
                running = false;
                break;
            default:
                std::cout << " [Error] Invalid choice. Please select an option between 1 and 16.\n";
                break;
        }
    }

    // Graceful process and IPC shutdown sequence
    std::cout << "\n========================================\n";
    std::cout << "Shutdown requested...\n";
    std::cout << "Stopping monitor process...\n";
    processManager.stopMonitor();
    std::cout << "Cleaning IPC resources...\n";
    inventory.saveToFile(DATA_FILE_PATH);
    sensor.disconnect();
    std::cout << "MediSave Edge stopped safely.\n";
    std::cout << "========================================\n";

    return 0;
}

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
#include "TemperatureMonitor.h"
#include "RedistributionEngine.h"
#include "SystemMonitor.h"
#include "ProcessManager.h"
#include "ThreadedMonitor.h"
#include "NetworkManager.h"

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
    if (std::cin.eof()) {
        g_shutdownRequested = 1;
        return;
    }
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

static std::string readLine(const std::string& prompt) {
    if (g_shutdownRequested || std::cin.eof()) {
        g_shutdownRequested = 1;
        return "";
    }
    std::cout << prompt;
    std::string val;
    if (!std::getline(std::cin, val)) {
        if (std::cin.eof()) {
            g_shutdownRequested = 1;
        }
    }
    return val;
}

static std::string readNonEmptyString(const std::string& prompt) {
    while (!g_shutdownRequested) {
        if (std::cin.eof()) {
            g_shutdownRequested = 1;
            return "";
        }
        std::string s = readLine(prompt);
        if (g_shutdownRequested) {
            return "";
        }
        if (!s.empty()) {
            return s;
        }
        if (std::cin.eof()) {
            g_shutdownRequested = 1;
            return "";
        }
        std::cout << " [Error] Input cannot be empty. Please try again.\n";
    }
    return "";
}

static int readInt(const std::string& prompt, int minVal = 0, int maxVal = 10000000) {
    while (!g_shutdownRequested) {
        if (std::cin.eof()) {
            g_shutdownRequested = 1;
            return 18;
        }
        std::cout << prompt;
        int val;
        if (std::cin >> val) {
            if (val >= minVal && val <= maxVal) {
                clearCin();
                return val;
            }
            std::cout << " [Error] Value must be between " << minVal << " and " << maxVal << ".\n";
        } else {
            if (std::cin.eof() || g_shutdownRequested) {
                g_shutdownRequested = 1;
                return 18;
            }
            std::cout << " [Error] Invalid integer input. Please try again.\n";
        }
        clearCin();
    }
    return 18;
}

static double readDouble(const std::string& prompt, double minVal = -50.0, double maxVal = 100.0) {
    while (!g_shutdownRequested) {
        if (std::cin.eof()) {
            g_shutdownRequested = 1;
            return 0.0;
        }
        std::cout << prompt;
        double val;
        if (std::cin >> val) {
            if (val >= minVal && val <= maxVal) {
                clearCin();
                return val;
            }
            std::cout << " [Error] Value must be between " << minVal << " and " << maxVal << ".\n";
        } else {
            if (std::cin.eof() || g_shutdownRequested) {
                g_shutdownRequested = 1;
                return 0.0;
            }
            std::cout << " [Error] Invalid numeric input. Please try again.\n";
        }
        clearCin();
    }
    return 0.0;
}

static std::string readDate(const std::string& prompt) {
    while (!g_shutdownRequested) {
        if (std::cin.eof()) {
            g_shutdownRequested = 1;
            return "2099-12-31";
        }
        std::string date = readNonEmptyString(prompt);
        if (g_shutdownRequested) {
            return "2099-12-31";
        }
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
    if (g_shutdownRequested) return;
    if (!Medicine::isValidId(id)) {
        std::cout << " [Error] ID must contain only alphanumeric characters, '-', or '_'.\n";
        return;
    }
    if (inv.findMedicineById(id) != nullptr) {
        std::cout << " [Error] Medicine ID '" << id << "' already exists in inventory!\n";
        return;
    }

    std::string name = readNonEmptyString("Enter Medicine Name: ");
    if (g_shutdownRequested) return;
    if (!Medicine::isValidName(name)) {
        std::cout << " [Error] Medicine name cannot be empty or contain '|' or control characters.\n";
        return;
    }
    std::string batch = readNonEmptyString("Enter Batch Number: ");
    if (g_shutdownRequested) return;
    if (!Medicine::isValidBatchNumber(batch)) {
        std::cout << " [Error] Batch number cannot be empty or contain '|' or control characters.\n";
        return;
    }
    int qty = readInt("Enter Initial Quantity: ", 0, 1000000);
    if (g_shutdownRequested) return;
    std::string expiry = readDate("Enter Expiry Date (YYYY-MM-DD): ");
    if (g_shutdownRequested) return;
    int minStock = readInt("Enter Minimum Required Stock: ", 0, 1000000);
    if (g_shutdownRequested) return;
    int maxStock = readInt("Enter Maximum Stock Capacity: ", minStock, 1000000);
    if (g_shutdownRequested) return;
    double minTemp = readDouble("Enter Minimum Storage Temperature (°C): ", -30.0, 50.0);
    if (g_shutdownRequested) return;
    double maxTemp = readDouble("Enter Maximum Storage Temperature (°C): ", minTemp, 50.0);
    if (g_shutdownRequested) return;

    try {
        Medicine med(id, name, batch, qty, expiry, minStock, maxStock, minTemp, maxTemp);
        if (inv.addMedicine(med)) {
            std::cout << " [Success] Medicine '" << name << "' added successfully.\n";
            inv.saveToFile(DATA_FILE_PATH);
        } else {
            std::cout << " [Error] Failed to add medicine record.\n";
        }
    } catch (const std::exception& e) {
        std::cout << " [Error] Exception during creation: " << e.what() << "\n";
    }
}

static void handleRemoveMedicine(InventoryManager& inv) {
    std::cout << "\n--- Remove Medicine ---\n";
    std::string id = readNonEmptyString("Enter Medicine ID to remove: ");
    if (g_shutdownRequested) return;
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
    if (g_shutdownRequested) return;
    Medicine* med = inv.findMedicineById(id);
    if (!med) {
        std::cout << " [Error] Medicine ID '" << id << "' not found.\n";
        return;
    }

    std::cout << "\nUpdating [" << med->getId() << "] " << med->getName() << "\n";
    std::cout << "Leave blank to keep existing value.\n";

    bool anyUpdated = false;
    bool updateFailed = false;

    std::string newName = readLine("New Name [" + med->getName() + "]: ");
    if (g_shutdownRequested) return;
    if (!newName.empty()) {
        if (!med->setName(newName)) {
            std::cout << " [Error] Invalid medicine name (cannot contain '|' or control characters).\n";
            updateFailed = true;
        } else {
            anyUpdated = true;
        }
    }

    std::string newBatch = readLine("New Batch [" + med->getBatchNumber() + "]: ");
    if (g_shutdownRequested) return;
    if (!newBatch.empty()) {
        if (!med->setBatchNumber(newBatch)) {
            std::cout << " [Error] Invalid batch number (cannot contain '|' or control characters).\n";
            updateFailed = true;
        } else {
            anyUpdated = true;
        }
    }

    std::string dateChoice = readLine("Update Expiry Date? (y/N): ");
    if (g_shutdownRequested) return;
    if (dateChoice == "y" || dateChoice == "Y") {
        std::string newExp = readDate("New Expiry Date (YYYY-MM-DD): ");
        if (g_shutdownRequested) return;
        if (!med->setExpiryDate(newExp)) {
            std::cout << " [Error] Invalid expiry date.\n";
            updateFailed = true;
        } else {
            anyUpdated = true;
        }
    }

    std::string boundsChoice = readLine("Update Stock Bounds? (y/N): ");
    if (g_shutdownRequested) return;
    if (boundsChoice == "y" || boundsChoice == "Y") {
        int minS = readInt("New Min Stock: ", 0, 1000000);
        if (g_shutdownRequested) return;
        int maxS = readInt("New Max Stock: ", minS, 1000000);
        if (g_shutdownRequested) return;
        if (!med->setStockBounds(minS, maxS)) {
            std::cout << " [Error] Invalid stock bounds.\n";
            updateFailed = true;
        } else {
            anyUpdated = true;
        }
    }

    std::string tempChoice = readLine("Update Temperature Bounds? (y/N): ");
    if (g_shutdownRequested) return;
    if (tempChoice == "y" || tempChoice == "Y") {
        double minT = readDouble("New Min Temp (°C): ", -30.0, 50.0);
        if (g_shutdownRequested) return;
        double maxT = readDouble("New Max Temp (°C): ", minT, 50.0);
        if (g_shutdownRequested) return;
        if (!med->setTemperatureBounds(minT, maxT)) {
            std::cout << " [Error] Invalid temperature bounds.\n";
            updateFailed = true;
        } else {
            anyUpdated = true;
        }
    }

    if (updateFailed) {
        std::cout << " [Error] Medicine update encountered errors; failed fields were not changed.\n";
    } else if (anyUpdated) {
        inv.saveToFile(DATA_FILE_PATH);
        std::cout << " [Success] Medicine details updated.\n";
    } else {
        std::cout << " [Notice] No changes made.\n";
    }
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

// Storage Temperature Handlers (Centralized TemperatureMonitor)
static void handleReadStorageTemperature(TemperatureMonitor& tempMon) {
    std::cout << "\n========================================\n";
    std::cout << "      READ STORAGE TEMPERATURE\n";
    std::cout << "========================================\n\n";

    if (tempMon.updateReading()) {
        TemperatureReading r = tempMon.getLatestReading();
        std::cout << "Sensor Device : " << tempMon.getDevicePath() << "\n";
        std::cout << "Temperature   : " << std::fixed << std::setprecision(2) << r.temperature << " C\n";
        std::cout << "Status        : " << r.status << "\n";
        std::cout << "Timestamp     : " << r.timestamp << "\n\n";

        if (r.status == "CRITICAL" || r.status == "WARNING" || r.status == "LOW") {
            std::cout << "ALERT: Storage condition requires attention.\n";
        }
    } else {
        std::cout << "[NOTICE] Storage sensor (" << tempMon.getDevicePath() << ") is UNAVAILABLE.\n";
        std::cout << "Please ensure the MediSave Linux kernel driver is loaded.\n";
    }
    std::cout << "========================================\n";
}

static void handleSetSimulatedTemperature(TemperatureMonitor& tempMon) {
    std::cout << "\n========================================\n";
    std::cout << "     SET SIMULATED TEMPERATURE\n";
    std::cout << "========================================\n\n";
    std::cout << "Driver Target : " << tempMon.getDevicePath() << "\n\n";

    double temp = readDouble("Enter simulated temperature (°C): ", -50.0, 100.0);
    std::cout << "\nDispatching temperature " << std::fixed << std::setprecision(2) << temp
              << " °C to kernel driver...\n";

    if (tempMon.setSimulatedTemperature(temp)) {
        TemperatureReading r = tempMon.getLatestReading();
        std::cout << " [Success] Temperature updated in kernel module.\n";
        std::cout << " New Temperature : " << std::fixed << std::setprecision(2) << r.temperature << " C\n";
        std::cout << " Resulting Status: " << r.status << "\n\n";

        if (r.status == "CRITICAL") {
            std::cout << "========================================\n";
            std::cout << "            STORAGE ALERT\n";
            std::cout << "========================================\n";
            std::cout << "Temperature: " << std::fixed << std::setprecision(2) << r.temperature << " C\n";
            std::cout << "Status: CRITICAL\n\n";
            std::cout << "Storage condition requires immediate attention.\n";
            std::cout << "========================================\n";
        } else if (r.status == "WARNING" || r.status == "LOW") {
            std::cout << "[STORAGE " << r.status << "] Storage condition requires attention.\n";
        }
    } else {
        std::cout << " [Error] Failed to write to " << tempMon.getDevicePath() << ".\n";
        std::cout << " Ensure kernel driver module is loaded (insmod) with write permissions.\n";
    }
    std::cout << "========================================\n";
}

static void handleShowStorageCondition(TemperatureMonitor& tempMon, const InventoryManager& inv) {
    tempMon.updateReading();
    tempMon.displayCondition();

    double temp = 0.0;
    std::string stat;
    std::string ts;
    if (tempMon.getLastCondition(temp, stat, ts)) {
        if (stat == "CRITICAL" || stat == "WARNING" || stat == "LOW") {
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

// Facility Communication Handlers
static void handleSendFacilityUpdate(NetworkManager& net, RedistributionEngine& redistEngine) {
    std::cout << "\n========================================\n";
    std::cout << "       SEND FACILITY UPDATE\n";
    std::cout << "========================================\n\n";

    std::string facility = readNonEmptyString("Enter Facility ID (e.g. Facility-A): ");
    std::string medicine = readNonEmptyString("Enter Medicine Name (e.g. Paracetamol): ");
    std::string batch = readNonEmptyString("Enter Batch Number (e.g. P2026A): ");
    int quantity = readInt("Enter Quantity (units): ", 1, 100000);

    std::cout << "\nUpdate Type:\n";
    std::cout << " 1. SURPLUS (Available for Redistribution)\n";
    std::cout << " 2. SHORTAGE (Urgent Supply Needed)\n";
    int typeChoice = readInt("Select Type (1-2): ", 1, 2);
    std::string type = (typeChoice == 1) ? "SURPLUS" : "SHORTAGE";

    std::cout << "\nSending payload: " << facility << "|" << medicine << "|" << batch << "|" << quantity << "|" << type << "\n";

    std::string ackResponse;
    std::string errorMsg;
    if (net.sendFacilityUpdate(facility, medicine, batch, quantity, type, ackResponse, errorMsg)) {
        std::cout << "\n [Success] Server acknowledgement received: " << ackResponse << "\n";
        std::cout << " Facility inventory update recorded successfully.\n";
    } else {
        std::cout << "\n [Notice] Direct TCP delivery returned: " << errorMsg << "\n";
        std::cout << " Recording update into local redistribution engine buffer.\n";
    }

    // Ingest into local engine buffer for subsequent redistribution analysis
    FacilityMessage msg{facility, medicine, batch, quantity, type};
    redistEngine.ingestFacilityMessage(msg);

    std::cout << "========================================\n";
}

static void handleAnalyzeRedistribution(RedistributionEngine& redistEngine, NetworkManager& net, const InventoryManager& inv) {
    // 1. Ingest any updates stored by background TCP server
    auto tcpUpdates = net.getStoredFacilityUpdates();
    redistEngine.ingestFacilityMessages(tcpUpdates);

    // 2. Ingest current local inventory as "Facility-Local"
    redistEngine.ingestLocalInventory("Facility-Local", inv.getAllMedicines());

    // 3. Generate and display matching recommendations
    redistEngine.displayRecommendations();
}

// System Health & Dashboard Handlers
static void handleShowSystemHealth(const SystemMonitor& sysMon, DeviceSensor& sensor,
                                   ThreadedMonitor& threadedMon, NetworkManager& net) {
    ServiceStatus s;
    s.driver = sensor.isConnected() ? "CONNECTED" : (sensor.connect() ? "CONNECTED" : "UNAVAILABLE");
    s.storageSensor = sensor.isConnected() ? "AVAILABLE" : "UNAVAILABLE";
    s.tcpServer = net.isServerRunning() ? "RUNNING" : "STOPPED";
    s.monitoring = threadedMon.isRunning() ? "RUNNING" : "STOPPED";

    sysMon.displaySystemHealth(s);
}

static void handleShowSystemDashboard(const SystemMonitor& sysMon, TemperatureMonitor& tempMon,
                                      const InventoryManager& inv, RedistributionEngine& redistEngine,
                                      NetworkManager& net, ThreadedMonitor& threadedMon, DeviceSensor& sensor) {
    bool readingOk = tempMon.updateReading();
    TemperatureReading r = tempMon.getLatestReading();

    // Refresh redistribution data
    auto tcpUpdates = net.getStoredFacilityUpdates();
    redistEngine.ingestFacilityMessages(tcpUpdates);
    redistEngine.ingestLocalInventory("Facility-Local", inv.getAllMedicines());
    auto recs = redistEngine.generateRecommendations();

    CpuInfo cpu;
    sysMon.getCpuInfo(cpu, 50);

    MemoryInfo mem;
    sysMon.getMemoryInfo(mem);

    double uptimeSec = 0.0;
    std::string uptimeStr;
    sysMon.getUptime(uptimeSec, uptimeStr);

    auto expiredMeds = inv.getExpiredMedicines();
    auto expiringMeds = inv.getExpiringSoonMedicines(30);
    auto lowStockMeds = inv.getLowStockMedicines();

    DashboardSnapshot snap;
    snap.temperatureValid = readingOk && r.valid && sensor.isConnected();
    snap.temperature = r.temperature;
    snap.storageStatus = (readingOk && r.valid) ? r.status : "UNAVAILABLE";

    snap.totalMedicines = static_cast<int>(inv.getMedicineCount());
    snap.lowStockCount = static_cast<int>(lowStockMeds.size());
    snap.expiringCount = static_cast<int>(expiringMeds.size());
    snap.expiredCount = static_cast<int>(expiredMeds.size());

    snap.facilityCount = static_cast<int>(redistEngine.getUniqueFacilityNamesCount());
    snap.shortageCount = static_cast<int>(redistEngine.getShortageCount());
    snap.surplusCount = static_cast<int>(redistEngine.getSurplusCount());
    snap.recommendationCount = static_cast<int>(recs.size());

    snap.cpuUsagePercent = cpu.usagePercent;
    snap.memoryUsagePercent = mem.usagePercent;
    snap.uptimeFormatted = uptimeStr;

    snap.driverStatus = sensor.isConnected() ? "CONNECTED" : (sensor.connect() ? "CONNECTED" : "UNAVAILABLE");
    snap.monitoringStatus = threadedMon.isRunning() ? "RUNNING" : "STOPPED";
    snap.tcpStatus = net.isServerRunning() ? "RUNNING" : "STOPPED";

    SystemMonitor::displaySystemDashboard(snap);
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
    StorageMonitor storageMonitor(sensor);
    TemperatureMonitor tempMonitor(sensor);
    RedistributionEngine redistributionEngine;
    SystemMonitor systemMonitor;
    ProcessManager processManager;
    ThreadedMonitor threadedMonitor(sensor, storageMonitor);
    NetworkManager networkManager(DEFAULT_TCP_HOST, DEFAULT_TCP_PORT, "Facility-Central");

    std::cout << "========================================\n";
    std::cout << "        MEDISAVE EDGE SYSTEM\n";
    std::cout << " Linux Medicine Storage & Redistribution\n";
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
        std::cout << " 9. Read Storage Temperature\n";
        std::cout << "10. Set Simulated Temperature\n";
        std::cout << "11. Show Storage Condition\n";
        std::cout << "12. Start Monitoring\n";
        std::cout << "13. Stop Monitoring\n";
        std::cout << "14. Send Facility Update\n";
        std::cout << "15. Analyze Redistribution\n";
        std::cout << "16. Show System Health\n";
        std::cout << "17. Show System Dashboard\n";
        std::cout << "18. Exit\n";
        std::cout << "========================================\n";

        int choice = readInt("Enter choice (1-18): ", 1, 18);

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
                AlertSystem::displayAlerts(inventory, ExpiryThresholds(), "", &storageMonitor);
                break;
            case 9:
                handleReadStorageTemperature(tempMonitor);
                break;
            case 10:
                handleSetSimulatedTemperature(tempMonitor);
                break;
            case 11:
                handleShowStorageCondition(tempMonitor, inventory);
                break;
            case 12:
                processManager.startMonitor();
                if (threadedMonitor.start(5)) {
                    networkManager.startServer();
                    std::cout << " [Success] Background monitoring active (Process monitor, Sensor thread, Alert thread, TCP Server).\n";
                } else {
                    std::cout << " [Info] In-process threaded monitoring is already active.\n";
                }
                break;
            case 13:
                processManager.stopMonitor();
                threadedMonitor.stop();
                std::cout << " [Success] Background monitoring stopped.\n";
                break;
            case 14:
                handleSendFacilityUpdate(networkManager, redistributionEngine);
                break;
            case 15:
                handleAnalyzeRedistribution(redistributionEngine, networkManager, inventory);
                break;
            case 16:
                handleShowSystemHealth(systemMonitor, sensor, threadedMonitor, networkManager);
                break;
            case 17:
                handleShowSystemDashboard(systemMonitor, tempMonitor, inventory, redistributionEngine, networkManager, threadedMonitor, sensor);
                break;
            case 18:
                running = false;
                break;
            default:
                std::cout << " [Error] Invalid choice. Please select an option between 1 and 18.\n";
                break;
        }
    }

    // Graceful process, thread, network, and IPC shutdown sequence
    std::cout << "\n========================================\n";
    std::cout << "Shutdown requested...\n";
    std::cout << "Stopping background monitoring threads...\n";
    threadedMonitor.stop();
    std::cout << "Stopping monitor processes...\n";
    processManager.stopMonitor();
    std::cout << "Stopping facility network services...\n";
    networkManager.stopServer();
    std::cout << "Saving inventory persistence file...\n";
    inventory.saveToFile(DATA_FILE_PATH);
    sensor.disconnect();
    std::cout << "MediSave Edge stopped safely.\n";
    std::cout << "========================================\n";

    return 0;
}

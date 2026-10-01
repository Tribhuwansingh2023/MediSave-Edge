#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <cstdio>
#include "medicine.h"
#include "inventory_manager.h"
#include "expiry_utils.h"
#include "alert_system.h"

static int totalTests = 0;
static int passedTests = 0;

#define TEST_ASSERT(cond, testName) \
    do { \
        totalTests++; \
        if (cond) { \
            passedTests++; \
            std::cout << " [PASS] " << testName << "\n"; \
        } else { \
            std::cerr << " [FAIL] " << testName << " (Line " << __LINE__ << ")\n"; \
        } \
    } while (0)

void testMedicineCreation() {
    // Valid medicine
    try {
        Medicine med("MED-001", "Amoxicillin", "B1001", 150, "2027-06-30", 50, 500, 15.0, 25.0);
        TEST_ASSERT(med.getId() == "MED-001", "Medicine creation with valid parameters");
        TEST_ASSERT(med.getQuantity() == 150, "Medicine quantity retrieval");
        TEST_ASSERT(!med.isLowStock(), "Medicine not low stock when qty > minStock");
    } catch (...) {
        TEST_ASSERT(false, "Medicine creation threw unexpected exception");
    }

    // Invalid parameters
    bool caughtInvalid = false;
    try {
        Medicine invalidMed("", "Bad Medicine", "B99", -10, "invalid-date", 100, 50, 30.0, 10.0);
    } catch (const std::invalid_argument&) {
        caughtInvalid = true;
    }
    TEST_ASSERT(caughtInvalid, "Medicine validation rejects negative quantity and bad dates");
}

void testAddAndDuplicate() {
    InventoryManager inv;
    Medicine m1("MED-01", "Paracetamol", "BATCH-A", 100, "2027-01-01", 20, 200, 2.0, 8.0);
    Medicine m2("MED-01", "Duplicate Para", "BATCH-B", 50, "2027-01-01", 20, 200, 2.0, 8.0);

    TEST_ASSERT(inv.addMedicine(m1) == true, "Add unique medicine ID to inventory");
    TEST_ASSERT(inv.getMedicineCount() == 1, "Inventory count increases after addition");
    TEST_ASSERT(inv.addMedicine(m2) == false, "Duplicate medicine ID correctly rejected");
    TEST_ASSERT(inv.getMedicineCount() == 1, "Inventory count unchanged after duplicate rejection");
}

void testSearchMedicine() {
    InventoryManager inv;
    inv.addMedicine(Medicine("M-01", "Insulin Glargine", "B-INS1", 40, "2026-11-01", 10, 100, 2.0, 8.0));
    inv.addMedicine(Medicine("M-02", "Insulin Lispro", "B-INS2", 30, "2026-12-01", 10, 100, 2.0, 8.0));
    inv.addMedicine(Medicine("M-03", "Ceftriaxone", "B-CEF1", 50, "2027-05-01", 20, 150, 15.0, 25.0));

    // Search by ID
    const Medicine* found = inv.findMedicineById("M-01");
    TEST_ASSERT(found != nullptr && found->getName() == "Insulin Glargine", "Search medicine by exact ID");

    const Medicine* notFound = inv.findMedicineById("NON-EXISTENT");
    TEST_ASSERT(notFound == nullptr, "Search non-existent ID returns nullptr");

    // Search by Name (case-insensitive substring)
    std::vector<Medicine> matches = inv.findMedicinesByName("insulin");
    TEST_ASSERT(matches.size() == 2, "Search medicines by name substring matches correct records");
}

void testStockUpdate() {
    InventoryManager inv;
    inv.addMedicine(Medicine("M-STK", "Ibuprofen", "B-IBU", 100, "2027-01-01", 30, 300, 15.0, 25.0));

    // Add stock
    bool added = inv.updateStock("M-STK", 50);
    const Medicine* med = inv.findMedicineById("M-STK");
    TEST_ASSERT(added && med->getQuantity() == 150, "Update stock by adding quantity");

    // Dispense stock
    bool dispensed = inv.updateStock("M-STK", -70);
    med = inv.findMedicineById("M-STK");
    TEST_ASSERT(dispensed && med->getQuantity() == 80, "Update stock by dispensing quantity");

    // Over-dispense rejection (cannot drop below 0)
    bool overDispense = inv.updateStock("M-STK", -200);
    med = inv.findMedicineById("M-STK");
    TEST_ASSERT(!overDispense && med->getQuantity() == 80, "Update stock rejects negative quantity");
}

void testRemoveMedicine() {
    InventoryManager inv;
    inv.addMedicine(Medicine("M-REM", "Aspirin", "B-ASP", 50, "2027-01-01", 10, 100, 15.0, 25.0));

    TEST_ASSERT(inv.removeMedicine("M-REM") == true, "Remove existing medicine by ID");
    TEST_ASSERT(inv.findMedicineById("M-REM") == nullptr, "Removed medicine is no longer in inventory");
    TEST_ASSERT(inv.removeMedicine("M-REM") == false, "Removing non-existent medicine returns false");
}

void testExpiryAndAlertCalculations() {
    std::string refDate = "2026-10-01";

    // Expired calculation
    int daysExp = ExpiryUtils::calculateDaysUntilExpiry("2026-09-25", refDate);
    TEST_ASSERT(daysExp == -6, "Calculate negative days for expired medicine");
    TEST_ASSERT(ExpiryUtils::getExpiryStatus(daysExp) == ExpiryStatus::EXPIRED, "Classify EXPIRED status");

    // Critical calculation (0 - 7 days)
    int daysCrit = ExpiryUtils::calculateDaysUntilExpiry("2026-10-04", refDate);
    TEST_ASSERT(daysCrit == 3, "Calculate 3 days remaining until expiry");
    TEST_ASSERT(ExpiryUtils::getExpiryStatus(daysCrit) == ExpiryStatus::CRITICAL, "Classify CRITICAL status for 3 days");

    // Warning calculation (8 - 30 days)
    int daysWarn = ExpiryUtils::calculateDaysUntilExpiry("2026-10-21", refDate);
    TEST_ASSERT(daysWarn == 20, "Calculate 20 days remaining until expiry");
    TEST_ASSERT(ExpiryUtils::getExpiryStatus(daysWarn) == ExpiryStatus::WARNING, "Classify WARNING status for 20 days");

    // Normal calculation (> 30 days)
    int daysNorm = ExpiryUtils::calculateDaysUntilExpiry("2027-05-15", refDate);
    TEST_ASSERT(daysNorm > 30, "Calculate > 30 days for safe medicine");
    TEST_ASSERT(ExpiryUtils::getExpiryStatus(daysNorm) == ExpiryStatus::NORMAL, "Classify NORMAL status");
}

void testInventoryAnalysisAndAlertQueue() {
    std::string refDate = "2026-10-01";
    InventoryManager inv;

    inv.addMedicine(Medicine("M-EXP", "Expired Syrups", "B-01", 10, "2026-09-20", 5, 50, 15.0, 25.0)); // Expired (-11d)
    inv.addMedicine(Medicine("M-CRIT", "Critical Vaccine", "B-02", 50, "2026-10-04", 10, 100, 2.0, 8.0)); // Critical (3d)
    inv.addMedicine(Medicine("M-LOW", "Low Stock Pills", "B-03", 2, "2027-01-01", 10, 100, 15.0, 25.0)); // Low stock (qty 2 <= min 10)
    inv.addMedicine(Medicine("M-NORM", "Safe Saline", "B-04", 500, "2027-10-01", 50, 1000, 15.0, 25.0)); // Safe

    // Check query filters
    auto expiredList = inv.getExpiredMedicines(refDate);
    TEST_ASSERT(expiredList.size() == 1 && expiredList[0].first.getId() == "M-EXP", "getExpiredMedicines detects expired records");

    auto expiringSoon = inv.getExpiringSoonMedicines(30, refDate);
    TEST_ASSERT(expiringSoon.size() == 1 && expiringSoon[0].first.getId() == "M-CRIT", "getExpiringSoonMedicines detects expiring records");

    auto lowStock = inv.getLowStockMedicines();
    TEST_ASSERT(lowStock.size() == 1 && lowStock[0].getId() == "M-LOW", "getLowStockMedicines detects depleted records");

    // Priority alert ordering: Expired must come before Critical, which must come before Low Stock
    auto alertQueue = AlertSystem::generateAlerts(inv, ExpiryThresholds(), refDate);
    TEST_ASSERT(!alertQueue.empty(), "Alert queue generated with active warnings");

    Alert topAlert = alertQueue.top();
    alertQueue.pop();
    TEST_ASSERT(topAlert.severity == "[CRITICAL]" && topAlert.medicineName == "Expired Syrups",
                "Priority Queue correctly prioritizes expired medicine at highest triage rank");

    Alert secondAlert = alertQueue.top();
    alertQueue.pop();
    TEST_ASSERT(secondAlert.medicineName == "Critical Vaccine",
                "Priority Queue ranks critical 3-day expiry immediately following expired stock");
}

void testFilePersistence() {
    const std::string testFile = "data/test_medicines.tmp";
    InventoryManager inv;

    inv.addMedicine(Medicine("T-01", "Test Medicine A", "BATCH-1", 100, "2027-01-01", 20, 200, 2.0, 8.0));
    inv.addMedicine(Medicine("T-02", "Test Medicine B", "BATCH-2", 45, "2026-11-15", 10, 100, 15.0, 25.0));

    // Save
    bool saved = inv.saveToFile(testFile);
    TEST_ASSERT(saved, "Save inventory to text file");

    // Load into new manager
    InventoryManager loadedInv;
    bool loaded = loadedInv.loadFromFile(testFile);
    TEST_ASSERT(loaded, "Load inventory from text file");
    TEST_ASSERT(loadedInv.getMedicineCount() == 2, "Loaded inventory has correct count");

    const Medicine* m1 = loadedInv.findMedicineById("T-01");
    TEST_ASSERT(m1 != nullptr && m1->getName() == "Test Medicine A" && m1->getQuantity() == 100,
                "Deserialized medicine attributes match original values");

    // Cleanup temp file
    std::remove(testFile.c_str());
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  MEDISAVE EDGE - UNIT TEST SUITE\n";
    std::cout << "========================================\n\n";

    testMedicineCreation();
    testAddAndDuplicate();
    testSearchMedicine();
    testStockUpdate();
    testRemoveMedicine();
    testExpiryAndAlertCalculations();
    testInventoryAnalysisAndAlertQueue();
    testFilePersistence();

    std::cout << "\n========================================\n";
    std::cout << " TEST RESULTS: " << passedTests << " / " << totalTests << " PASSED\n";
    std::cout << "========================================\n";

    return (passedTests == totalTests) ? 0 : 1;
}

#ifndef INVENTORY_MANAGER_H
#define INVENTORY_MANAGER_H

#include "medicine.h"
#include "expiry_utils.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <utility>

/**
 * @class InventoryManager
 * @brief Manages medicine stock, lookups, expiry queries, and file persistence.
 * 
 * Uses std::unordered_map for O(1) average-time lookups by medicine ID.
 */
class InventoryManager {
private:
    std::unordered_map<std::string, Medicine> medicines;

public:
    InventoryManager() = default;

    // Core CRUD operations
    bool addMedicine(const Medicine& med);
    bool removeMedicine(const std::string& id);
    bool updateMedicine(const Medicine& med);
    const Medicine* findMedicineById(const std::string& id) const;
    Medicine* findMedicineById(const std::string& id);
    std::vector<Medicine> findMedicinesByName(const std::string& query) const;
    std::vector<Medicine> getAllMedicines() const;
    size_t getMedicineCount() const;
    void clear();

    // Stock management
    bool updateStock(const std::string& id, int deltaQuantity);

    // Monitoring and analysis queries
    std::vector<Medicine> getLowStockMedicines() const;
    std::vector<std::pair<Medicine, int>> getExpiredMedicines(const std::string& refDate = "") const;
    std::vector<std::pair<Medicine, int>> getExpiringSoonMedicines(int warningDays = 30, const std::string& refDate = "") const;

    // File persistence (data/medicines.txt)
    bool loadFromFile(const std::string& filepath);
    bool saveToFile(const std::string& filepath) const;
};

#endif // INVENTORY_MANAGER_H

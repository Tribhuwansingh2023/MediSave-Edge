#include "inventory_manager.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>

static std::string toLowerString(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lower;
}

bool InventoryManager::addMedicine(const Medicine& med) {
    if (medicines.find(med.getId()) != medicines.end()) {
        return false; // Duplicate ID
    }
    medicines[med.getId()] = med;
    return true;
}

bool InventoryManager::removeMedicine(const std::string& id) {
    auto it = medicines.find(id);
    if (it == medicines.end()) {
        return false;
    }
    medicines.erase(it);
    return true;
}

bool InventoryManager::updateMedicine(const Medicine& med) {
    auto it = medicines.find(med.getId());
    if (it == medicines.end()) {
        return false;
    }
    it->second = med;
    return true;
}

const Medicine* InventoryManager::findMedicineById(const std::string& id) const {
    auto it = medicines.find(id);
    if (it != medicines.end()) {
        return &(it->second);
    }
    return nullptr;
}

Medicine* InventoryManager::findMedicineById(const std::string& id) {
    auto it = medicines.find(id);
    if (it != medicines.end()) {
        return &(it->second);
    }
    return nullptr;
}

std::vector<Medicine> InventoryManager::findMedicinesByName(const std::string& query) const {
    std::vector<Medicine> results;
    std::string lowerQuery = toLowerString(query);

    for (const auto& [id, med] : medicines) {
        std::string lowerName = toLowerString(med.getName());
        if (lowerName.find(lowerQuery) != std::string::npos) {
            results.push_back(med);
        }
    }
    return results;
}

std::vector<Medicine> InventoryManager::getAllMedicines() const {
    std::vector<Medicine> list;
    list.reserve(medicines.size());
    for (const auto& [id, med] : medicines) {
        list.push_back(med);
    }
    return list;
}

size_t InventoryManager::getMedicineCount() const {
    return medicines.size();
}

void InventoryManager::clear() {
    medicines.clear();
}

bool InventoryManager::updateStock(const std::string& id, int deltaQuantity) {
    Medicine* med = findMedicineById(id);
    if (!med) return false;

    int newQty = med->getQuantity() + deltaQuantity;
    if (newQty < 0) {
        return false; // Negative inventory disallowed
    }
    med->setQuantity(newQty);
    return true;
}

std::vector<Medicine> InventoryManager::getLowStockMedicines() const {
    std::vector<Medicine> results;
    for (const auto& [id, med] : medicines) {
        if (med.isLowStock()) {
            results.push_back(med);
        }
    }
    return results;
}

std::vector<std::pair<Medicine, int>> InventoryManager::getExpiredMedicines(const std::string& refDate) const {
    std::vector<std::pair<Medicine, int>> results;
    for (const auto& [id, med] : medicines) {
        int days = ExpiryUtils::calculateDaysUntilExpiry(med.getExpiryDate(), refDate);
        if (days < 0) {
            results.push_back({med, days});
        }
    }
    return results;
}

std::vector<std::pair<Medicine, int>> InventoryManager::getExpiringSoonMedicines(int warningDays, const std::string& refDate) const {
    std::vector<std::pair<Medicine, int>> results;
    for (const auto& [id, med] : medicines) {
        int days = ExpiryUtils::calculateDaysUntilExpiry(med.getExpiryDate(), refDate);
        if (days >= 0 && days <= warningDays) {
            results.push_back({med, days});
        }
    }
    return results;
}

bool InventoryManager::loadFromFile(const std::string& filepath) {
    std::ifstream inFile(filepath);
    if (!inFile.is_open()) {
        return false;
    }

    medicines.clear();
    std::string line;
    while (std::getline(inFile, line)) {
        if (line.empty() || line[0] == '#') {
            continue; // Skip comments and blank lines
        }

        Medicine med;
        if (Medicine::deserialize(line, med)) {
            medicines[med.getId()] = med;
        } else {
            std::cerr << "[Warning] Skipping malformed line in " << filepath << ": " << line << "\n";
        }
    }

    inFile.close();
    return true;
}

bool InventoryManager::saveToFile(const std::string& filepath) const {
    std::ofstream outFile(filepath);
    if (!outFile.is_open()) {
        return false;
    }

    outFile << "# ==============================================================================\n";
    outFile << "# MediSave Edge Inventory Storage File\n";
    outFile << "# Schema: ID|Name|Batch|Quantity|Expiry|MinStock|MaxStock|MinTemp|MaxTemp\n";
    outFile << "# ==============================================================================\n";

    for (const auto& [id, med] : medicines) {
        outFile << med.serialize() << "\n";
    }

    outFile.close();
    return true;
}

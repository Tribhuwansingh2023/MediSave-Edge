#include "RedistributionEngine.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <set>
#include <cctype>

static std::string toUpper(const std::string& str) {
    std::string out = str;
    for (char& c : out) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return out;
}

StockClassification RedistributionEngine::classifyStock(int quantity, int minStock, int maxStock) {
    if (quantity < minStock) {
        return StockClassification::SHORTAGE;
    } else if (maxStock > 0 && quantity > maxStock) {
        return StockClassification::SURPLUS;
    } else {
        return StockClassification::NORMAL;
    }
}

std::string RedistributionEngine::classificationToString(StockClassification classification) {
    switch (classification) {
        case StockClassification::SHORTAGE:
            return "SHORTAGE";
        case StockClassification::SURPLUS:
            return "SURPLUS";
        case StockClassification::NORMAL:
        default:
            return "NORMAL";
    }
}

bool RedistributionEngine::isMedicineMatch(const FacilityStock& a, const FacilityStock& b) {
    if (!a.medicineId.empty() && !b.medicineId.empty()) {
        if (toUpper(a.medicineId) == toUpper(b.medicineId)) {
            return true;
        }
    }
    if (!a.medicineName.empty() && !b.medicineName.empty()) {
        if (toUpper(a.medicineName) == toUpper(b.medicineName)) {
            return true;
        }
    }
    return false;
}

void RedistributionEngine::addFacilityStock(const FacilityStock& stock) {
    std::lock_guard<std::mutex> lock(engineMutex);

    // Update existing if matching facility, medicine, and batch
    for (auto& item : facilities) {
        if (item.facilityName == stock.facilityName &&
            (item.medicineId == stock.medicineId || item.medicineName == stock.medicineName) &&
            item.batchNumber == stock.batchNumber) {
            item = stock;
            return;
        }
    }
    facilities.push_back(stock);
}

void RedistributionEngine::ingestFacilityMessage(const FacilityMessage& msg) {
    FacilityStock stock;
    stock.facilityName = msg.facility;
    stock.medicineName = msg.medicine;
    stock.medicineId = msg.medicine;
    stock.batchNumber = msg.batch;
    stock.quantity = msg.quantity;

    if (toUpper(msg.type) == "SURPLUS") {
        stock.minimumRequired = 50;
        stock.maximumCapacity = 100;
        if (stock.quantity <= stock.minimumRequired) {
            stock.quantity = stock.minimumRequired + msg.quantity;
        }
    } else if (toUpper(msg.type) == "SHORTAGE") {
        stock.minimumRequired = 50;
        stock.maximumCapacity = 100;
        if (stock.quantity >= stock.minimumRequired) {
            stock.quantity = std::max(0, stock.minimumRequired - msg.quantity);
        }
    } else {
        stock.minimumRequired = 50;
        stock.maximumCapacity = 100;
    }

    addFacilityStock(stock);
}

void RedistributionEngine::ingestFacilityMessages(const std::vector<FacilityMessage>& messages) {
    for (const auto& msg : messages) {
        ingestFacilityMessage(msg);
    }
}

void RedistributionEngine::ingestLocalInventory(const std::string& facilityName, const std::vector<Medicine>& meds) {
    for (const auto& med : meds) {
        FacilityStock stock;
        stock.facilityName = facilityName;
        stock.medicineId = med.getId();
        stock.medicineName = med.getName();
        stock.batchNumber = med.getBatchNumber();
        stock.quantity = med.getQuantity();
        stock.minimumRequired = med.getMinStock();
        stock.maximumCapacity = med.getMaxStock();
        stock.expiryDate = med.getExpiryDate();
        addFacilityStock(stock);
    }
}

void RedistributionEngine::clearFacilities() {
    std::lock_guard<std::mutex> lock(engineMutex);
    facilities.clear();
}

std::vector<FacilityStock> RedistributionEngine::getAllFacilityStocks() const {
    std::lock_guard<std::mutex> lock(engineMutex);
    return facilities;
}

size_t RedistributionEngine::getFacilityCount() const {
    std::lock_guard<std::mutex> lock(engineMutex);
    return facilities.size();
}

size_t RedistributionEngine::getUniqueFacilityNamesCount() const {
    std::lock_guard<std::mutex> lock(engineMutex);
    std::set<std::string> names;
    for (const auto& f : facilities) {
        names.insert(f.facilityName);
    }
    return names.size();
}

size_t RedistributionEngine::getShortageCount() const {
    std::lock_guard<std::mutex> lock(engineMutex);
    size_t count = 0;
    for (const auto& f : facilities) {
        if (f.quantity < f.minimumRequired) {
            count++;
        }
    }
    return count;
}

size_t RedistributionEngine::getSurplusCount() const {
    std::lock_guard<std::mutex> lock(engineMutex);
    size_t count = 0;
    for (const auto& f : facilities) {
        if (f.quantity > f.minimumRequired) {
            count++;
        }
    }
    return count;
}

std::vector<RedistributionRecommendation> RedistributionEngine::generateRecommendations() const {
    std::lock_guard<std::mutex> lock(engineMutex);
    std::vector<RedistributionRecommendation> recommendations;

    struct WorkingNode {
        FacilityStock stock;
        int balance{0}; // positive = surplus, negative = shortage
    };

    std::vector<WorkingNode> sources;
    std::vector<WorkingNode> destinations;

    for (const auto& f : facilities) {
        int surplus = f.quantity - f.minimumRequired;
        int shortage = f.minimumRequired - f.quantity;

        if (surplus > 0) {
            WorkingNode node;
            node.stock = f;
            node.balance = surplus;
            sources.push_back(node);
        } else if (shortage > 0) {
            WorkingNode node;
            node.stock = f;
            node.balance = shortage;
            destinations.push_back(node);
        }
    }

    // Deterministic priority ordering for destinations:
    // 1. Larger shortage first (balance descending)
    // 2. Earlier expiry first (if expiry provided)
    // 3. Deterministic tie-breaker by medicine name, ID, facility name
    std::sort(destinations.begin(), destinations.end(), [](const WorkingNode& a, const WorkingNode& b) {
        if (a.balance != b.balance) {
            return a.balance > b.balance; // Larger shortage first
        }
        if (!a.stock.expiryDate.empty() && !b.stock.expiryDate.empty()) {
            if (a.stock.expiryDate != b.stock.expiryDate) {
                return a.stock.expiryDate < b.stock.expiryDate; // Earlier expiry first
            }
        }
        if (a.stock.medicineName != b.stock.medicineName) {
            return a.stock.medicineName < b.stock.medicineName;
        }
        if (a.stock.medicineId != b.stock.medicineId) {
            return a.stock.medicineId < b.stock.medicineId;
        }
        return a.stock.facilityName < b.stock.facilityName;
    });

    // Deterministic ordering for sources:
    // 1. Larger surplus first
    // 2. Compatible batch / earlier expiry if any
    // 3. Source facility name
    std::sort(sources.begin(), sources.end(), [](const WorkingNode& a, const WorkingNode& b) {
        if (a.balance != b.balance) {
            return a.balance > b.balance;
        }
        if (a.stock.medicineName != b.stock.medicineName) {
            return a.stock.medicineName < b.stock.medicineName;
        }
        return a.stock.facilityName < b.stock.facilityName;
    });

    // Match surplus with shortages
    for (auto& dest : destinations) {
        for (auto& src : sources) {
            if (dest.balance <= 0) {
                break;
            }
            if (src.balance <= 0) {
                continue;
            }
            if (src.stock.facilityName == dest.stock.facilityName) {
                continue;
            }
            if (!isMedicineMatch(src.stock, dest.stock)) {
                continue;
            }

            int transfer = std::min(src.balance, dest.balance);
            if (transfer <= 0) {
                continue;
            }

            RedistributionRecommendation rec;
            rec.medicineId = !dest.stock.medicineId.empty() ? dest.stock.medicineId : src.stock.medicineId;
            rec.medicineName = !dest.stock.medicineName.empty() ? dest.stock.medicineName : src.stock.medicineName;
            rec.batchNumber = !src.stock.batchNumber.empty() ? src.stock.batchNumber : dest.stock.batchNumber;
            rec.sourceFacility = src.stock.facilityName;
            rec.sourceSurplus = src.balance;
            rec.destinationFacility = dest.stock.facilityName;
            rec.destinationShortage = dest.balance;
            rec.suggestedTransfer = transfer;
            rec.reason = "Destination facility has a shortage while source facility has sufficient surplus.";
            rec.expiryDate = src.stock.expiryDate;
            rec.priorityScore = dest.balance * 10;

            recommendations.push_back(rec);

            src.balance -= transfer;
            dest.balance -= transfer;
        }
    }

    return recommendations;
}

void RedistributionEngine::displayRecommendations() const {
    auto recs = generateRecommendations();
    size_t uniqueFacs = getUniqueFacilityNamesCount();
    size_t totalStocks = getFacilityCount();

    std::cout << "\n========================================\n";
    std::cout << "       REDISTRIBUTION ANALYSIS\n";
    std::cout << "========================================\n\n";
    std::cout << "Facilities received : " << uniqueFacs << "\n";
    std::cout << "Stock records       : " << totalStocks << "\n";
    std::cout << "Recommendations     : " << recs.size() << "\n\n";

    if (recs.empty()) {
        std::cout << "----------------------------------------\n";
        std::cout << "No redistribution recommendation found.\n";
        std::cout << "All facilities are at normal levels or no\n";
        std::cout << "matching surplus/shortage pairs exist.\n";
        std::cout << "----------------------------------------\n";
    } else {
        int idx = 1;
        for (const auto& rec : recs) {
            std::cout << "----------------------------------------\n";
            std::cout << "Recommendation #" << idx++ << "\n";
            std::cout << "----------------------------------------\n";
            std::cout << "Medicine : " << rec.medicineName;
            if (!rec.medicineId.empty() && rec.medicineId != rec.medicineName) {
                std::cout << " (" << rec.medicineId << ")";
            }
            std::cout << "\n";
            if (!rec.batchNumber.empty()) {
                std::cout << "Batch    : " << rec.batchNumber << "\n";
            }
            std::cout << "\nSource Facility:\n";
            std::cout << "  " << rec.sourceFacility << "\n";
            std::cout << "  Available Surplus : " << rec.sourceSurplus << " units\n";
            std::cout << "\nDestination Facility:\n";
            std::cout << "  " << rec.destinationFacility << "\n";
            std::cout << "  Required Shortage : " << rec.destinationShortage << " units\n\n";
            std::cout << "Suggested Transfer:\n";
            std::cout << "  " << rec.suggestedTransfer << " units\n\n";
            std::cout << "Reason:\n";
            std::cout << "  " << rec.reason << "\n";
            std::cout << "----------------------------------------\n\n";
        }
    }

    std::cout << "[ADVISORY NOTICE]\n";
    std::cout << "Redistribution recommendations are decision-support outputs.\n";
    std::cout << "No automatic stock transfer performed.\n";
    std::cout << "========================================\n";
}

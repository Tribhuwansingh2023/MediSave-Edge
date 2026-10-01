#ifndef REDISTRIBUTION_ENGINE_H
#define REDISTRIBUTION_ENGINE_H

#include "TcpProtocol.h"
#include "medicine.h"

#include <string>
#include <vector>
#include <mutex>
#include <algorithm>

/**
 * @enum StockClassification
 * @brief Categorization of inventory levels relative to facility thresholds.
 */
enum class StockClassification {
    SHORTAGE,
    NORMAL,
    SURPLUS
};

/**
 * @struct FacilityStock
 * @brief In-memory representation of medicine stock across distributed healthcare facilities.
 */
struct FacilityStock {
    std::string facilityName;
    std::string medicineId;
    std::string medicineName;
    std::string batchNumber;
    int quantity{0};
    int minimumRequired{0};
    int maximumCapacity{0};
    std::string expiryDate; // Format: YYYY-MM-DD (optional)
};

/**
 * @struct RedistributionRecommendation
 * @brief Actionable, advisory re-allocation proposal from a surplus facility to a shortage facility.
 */
struct RedistributionRecommendation {
    std::string medicineId;
    std::string medicineName;
    std::string batchNumber;
    std::string sourceFacility;
    int sourceSurplus{0};
    std::string destinationFacility;
    int destinationShortage{0};
    int suggestedTransfer{0};
    std::string reason;
    std::string expiryDate;
    int priorityScore{0};
};

/**
 * @class RedistributionEngine
 * @brief Deterministic decision-support engine matching surplus and shortage facilities.
 *
 * Constraints & Guarantees:
 * - Recommendations are advisory software outputs and do NOT execute automatic physical transfers.
 * - Deterministic priority ordering: larger shortages first, earlier expiry first, deterministic ID ties.
 * - Transfer quantity = min(surplus, shortage); never negative, zero, or exceeding available surplus.
 * - Thread-safe access for background TCP worker ingestion and CLI queries.
 */
class RedistributionEngine {
private:
    mutable std::mutex engineMutex;
    std::vector<FacilityStock> facilities;

    // Internal matching helper
    static bool isMedicineMatch(const FacilityStock& a, const FacilityStock& b);

public:
    RedistributionEngine() = default;

    // Stock classification helpers
    static StockClassification classifyStock(int quantity, int minStock, int maxStock);
    static std::string classificationToString(StockClassification classification);

    // Facility stock ingestion
    void addFacilityStock(const FacilityStock& stock);
    void ingestFacilityMessage(const FacilityMessage& msg);
    void ingestFacilityMessages(const std::vector<FacilityMessage>& messages);
    void ingestLocalInventory(const std::string& facilityName, const std::vector<Medicine>& meds);
    void clearFacilities();

    // Query state
    std::vector<FacilityStock> getAllFacilityStocks() const;
    size_t getFacilityCount() const;
    size_t getUniqueFacilityNamesCount() const;
    size_t getShortageCount() const;
    size_t getSurplusCount() const;

    // Recommendation generation
    std::vector<RedistributionRecommendation> generateRecommendations() const;

    // Formatted CLI reporting
    void displayRecommendations() const;
};

#endif // REDISTRIBUTION_ENGINE_H

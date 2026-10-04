#ifndef MEDICINE_H
#define MEDICINE_H

#include <string>
#include <iostream>

/**
 * @class Medicine
 * @brief Represents an individual medicine entity in the MediSave Edge system.
 * 
 * Encapsulates identification, batching, quantity, shelf life (expiry),
 * stock thresholds, and environmental temperature requirements.
 */
class Medicine {
private:
    std::string id;
    std::string name;
    std::string batchNumber;
    int quantity;
    std::string expiryDate;     // Format: YYYY-MM-DD
    int minStock;
    int maxStock;
    double minTemperature;      // Storage temperature range in °C
    double maxTemperature;

public:
    // Default constructor
    Medicine();

    // Parameterized constructor
    Medicine(const std::string& id,
             const std::string& name,
             const std::string& batchNumber,
             int quantity,
             const std::string& expiryDate,
             int minStock,
             int maxStock,
             double minTemperature,
             double maxTemperature);

    // Getters
    std::string getId() const;
    std::string getName() const;
    std::string getBatchNumber() const;
    int getQuantity() const;
    std::string getExpiryDate() const;
    int getMinStock() const;
    int getMaxStock() const;
    double getMinTemperature() const;
    double getMaxTemperature() const;

    // Setters with validation
    bool setName(const std::string& name);
    bool setBatchNumber(const std::string& batch);
    bool setQuantity(int qty);
    bool setExpiryDate(const std::string& date);
    bool setStockBounds(int minS, int maxS);
    bool setTemperatureBounds(double minT, double maxT);

    // Business logic helpers
    bool isLowStock() const;
    bool isOverStock() const;
    bool isOutOfStock() const;
    bool isTemperatureViolated(double currentTemp) const;

    // Text file persistence (Pipe-delimited: ID|Name|Batch|Quantity|Expiry|MinStock|MaxStock|MinTemp|MaxTemp)
    std::string serialize() const;
    static bool deserialize(const std::string& line, Medicine& outMed);

    // Static validators
    static bool isValidId(const std::string& id);
    static bool isValidName(const std::string& name);
    static bool isValidBatchNumber(const std::string& batch);
    static bool isValidDate(const std::string& date);
    static bool isValidQuantity(int qty);
    static bool isValidStockBounds(int minS, int maxS);
    static bool isValidTemperatureBounds(double minT, double maxT);
};

#endif // MEDICINE_H

#include "medicine.h"
#include <sstream>
#include <vector>
#include <stdexcept>
#include <iomanip>
#include <cctype>

Medicine::Medicine()
    : id(""),
      name(""),
      batchNumber(""),
      quantity(0),
      expiryDate("2099-12-31"),
      minStock(0),
      maxStock(1000),
      minTemperature(2.0),
      maxTemperature(8.0) {}

Medicine::Medicine(const std::string& id,
                   const std::string& name,
                   const std::string& batchNumber,
                   int quantity,
                   const std::string& expiryDate,
                   int minStock,
                   int maxStock,
                   double minTemperature,
                   double maxTemperature)
    : id(id),
      name(name),
      batchNumber(batchNumber),
      quantity(quantity),
      expiryDate(expiryDate),
      minStock(minStock),
      maxStock(maxStock),
      minTemperature(minTemperature),
      maxTemperature(maxTemperature) {
    if (!isValidId(id)) {
        throw std::invalid_argument("Medicine ID cannot be empty or contain invalid characters.");
    }
    if (!isValidName(name)) {
        throw std::invalid_argument("Medicine name cannot be empty, contain '|', or contain control characters.");
    }
    if (!isValidBatchNumber(batchNumber)) {
        throw std::invalid_argument("Medicine batch number cannot be empty, contain '|', or contain control characters.");
    }
    if (!isValidQuantity(quantity)) {
        throw std::invalid_argument("Medicine quantity cannot be negative.");
    }
    if (!isValidDate(expiryDate)) {
        throw std::invalid_argument("Invalid expiry date format. Expected YYYY-MM-DD.");
    }
    if (!isValidStockBounds(minStock, maxStock)) {
        throw std::invalid_argument("Invalid stock bounds: minStock must be non-negative and <= maxStock.");
    }
    if (!isValidTemperatureBounds(minTemperature, maxTemperature)) {
        throw std::invalid_argument("Invalid temperature bounds: minTemperature must be <= maxTemperature.");
    }
}

std::string Medicine::getId() const { return id; }
std::string Medicine::getName() const { return name; }
std::string Medicine::getBatchNumber() const { return batchNumber; }
int Medicine::getQuantity() const { return quantity; }
std::string Medicine::getExpiryDate() const { return expiryDate; }
int Medicine::getMinStock() const { return minStock; }
int Medicine::getMaxStock() const { return maxStock; }
double Medicine::getMinTemperature() const { return minTemperature; }
double Medicine::getMaxTemperature() const { return maxTemperature; }

bool Medicine::setName(const std::string& newName) {
    if (!isValidName(newName)) return false;
    name = newName;
    return true;
}

bool Medicine::setBatchNumber(const std::string& batch) {
    if (!isValidBatchNumber(batch)) return false;
    batchNumber = batch;
    return true;
}

bool Medicine::setQuantity(int qty) {
    if (!isValidQuantity(qty)) return false;
    quantity = qty;
    return true;
}

bool Medicine::setExpiryDate(const std::string& date) {
    if (!isValidDate(date)) return false;
    expiryDate = date;
    return true;
}

bool Medicine::setStockBounds(int minS, int maxS) {
    if (!isValidStockBounds(minS, maxS)) return false;
    minStock = minS;
    maxStock = maxS;
    return true;
}

bool Medicine::setTemperatureBounds(double minT, double maxT) {
    if (!isValidTemperatureBounds(minT, maxT)) return false;
    minTemperature = minT;
    maxTemperature = maxT;
    return true;
}

bool Medicine::isLowStock() const {
    return quantity <= minStock;
}

bool Medicine::isOverStock() const {
    return quantity > maxStock;
}

bool Medicine::isOutOfStock() const {
    return quantity == 0;
}

bool Medicine::isTemperatureViolated(double currentTemp) const {
    return (currentTemp < minTemperature || currentTemp > maxTemperature);
}

std::string Medicine::serialize() const {
    std::ostringstream oss;
    oss << id << "|"
        << name << "|"
        << batchNumber << "|"
        << quantity << "|"
        << expiryDate << "|"
        << minStock << "|"
        << maxStock << "|"
        << std::fixed << std::setprecision(1) << minTemperature << "|"
        << std::fixed << std::setprecision(1) << maxTemperature;
    return oss.str();
}

bool Medicine::deserialize(const std::string& line, Medicine& outMed) {
    if (line.empty()) return false;

    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;

    while (std::getline(ss, token, '|')) {
        tokens.push_back(token);
    }

    if (tokens.size() != 9) {
        return false;
    }

    try {
        std::string parsedId = tokens[0];
        std::string parsedName = tokens[1];
        std::string parsedBatch = tokens[2];
        int parsedQty = std::stoi(tokens[3]);
        std::string parsedExpiry = tokens[4];
        int parsedMinStock = std::stoi(tokens[5]);
        int parsedMaxStock = std::stoi(tokens[6]);
        double parsedMinTemp = std::stod(tokens[7]);
        double parsedMaxTemp = std::stod(tokens[8]);

        if (!isValidId(parsedId) ||
            !isValidName(parsedName) ||
            !isValidBatchNumber(parsedBatch) ||
            !isValidQuantity(parsedQty) ||
            !isValidDate(parsedExpiry) ||
            !isValidStockBounds(parsedMinStock, parsedMaxStock) ||
            !isValidTemperatureBounds(parsedMinTemp, parsedMaxTemp)) {
            return false;
        }

        outMed = Medicine(parsedId,
                          parsedName,
                          parsedBatch,
                          parsedQty,
                          parsedExpiry,
                          parsedMinStock,
                          parsedMaxStock,
                          parsedMinTemp,
                          parsedMaxTemp);
        return true;
    } catch (...) {
        return false;
    }
}

bool Medicine::isValidId(const std::string& id) {
    if (id.empty()) return false;
    for (char c : id) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_') {
            return false;
        }
    }
    return true;
}

bool Medicine::isValidName(const std::string& name) {
    if (name.empty()) return false;
    bool hasNonSpace = false;
    for (char c : name) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (c == '|' || std::iscntrl(uc)) {
            return false;
        }
        if (!std::isspace(uc)) {
            hasNonSpace = true;
        }
    }
    return hasNonSpace;
}

bool Medicine::isValidBatchNumber(const std::string& batch) {
    if (batch.empty()) return false;
    bool hasNonSpace = false;
    for (char c : batch) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (c == '|' || std::iscntrl(uc)) {
            return false;
        }
        if (!std::isspace(uc)) {
            hasNonSpace = true;
        }
    }
    return hasNonSpace;
}

bool Medicine::isValidDate(const std::string& date) {
    // Format must be YYYY-MM-DD (10 characters)
    if (date.length() != 10) return false;
    if (date[4] != '-' || date[7] != '-') return false;

    for (size_t i = 0; i < date.length(); ++i) {
        if (i == 4 || i == 7) continue;
        if (!std::isdigit(static_cast<unsigned char>(date[i]))) return false;
    }

    try {
        int year = std::stoi(date.substr(0, 4));
        int month = std::stoi(date.substr(5, 2));
        int day = std::stoi(date.substr(8, 2));

        if (year < 2000 || year > 2100) return false;
        if (month < 1 || month > 12) return false;
        if (day < 1 || day > 31) return false;

        // Month day limits
        if ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30) return false;
        if (month == 2) {
            bool isLeap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
            if (day > (isLeap ? 29 : 28)) return false;
        }
    } catch (...) {
        return false;
    }

    return true;
}

bool Medicine::isValidQuantity(int qty) {
    return qty >= 0;
}

bool Medicine::isValidStockBounds(int minS, int maxS) {
    return (minS >= 0 && maxS >= minS);
}

bool Medicine::isValidTemperatureBounds(double minT, double maxT) {
    return (minT <= maxT);
}

#ifndef TCP_PROTOCOL_H
#define TCP_PROTOCOL_H

#include <string>
#include <vector>
#include <sstream>

constexpr int DEFAULT_TCP_PORT = 5000;
constexpr const char* DEFAULT_TCP_HOST = "127.0.0.1";

/**
 * @struct FacilityMessage
 * @brief Inter-facility medicine inventory update transferred across TCP.
 *
 * Prepared for consumption by the Task 7 Redistribution Decision Engine.
 */
struct FacilityMessage {
    std::string facility;  // e.g. "Facility-A"
    std::string medicine;  // e.g. "Paracetamol"
    std::string batch;     // e.g. "P2026A"
    int quantity{0};       // e.g. 150
    std::string type;      // "SURPLUS" or "SHORTAGE"
};

/**
 * @brief Formats a FacilityMessage into the pipe-delimited wire protocol:
 *        FACILITY|MEDICINE|BATCH|QUANTITY|TYPE\n
 */
inline std::string serializeFacilityMessage(const FacilityMessage& msg) {
    std::ostringstream oss;
    oss << msg.facility << "|"
        << msg.medicine << "|"
        << msg.batch << "|"
        << msg.quantity << "|"
        << msg.type << "\n";
    return oss.str();
}

/**
 * @brief Parses an incoming pipe-delimited message payload.
 *
 * Validates:
 * - Exactly 5 fields: FACILITY|MEDICINE|BATCH|QUANTITY|TYPE
 * - Facility must not be empty
 * - Medicine must not be empty
 * - Batch must not be empty
 * - Quantity must be numeric and > 0
 * - Type must be either "SURPLUS" or "SHORTAGE"
 */
inline bool parseFacilityMessage(const std::string& raw, FacilityMessage& msg, std::string* errorReason = nullptr) {
    if (raw.empty()) {
        if (errorReason) *errorReason = "Empty payload";
        return false;
    }

    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(raw);

    while (std::getline(tokenStream, token, '|')) {
        tokens.push_back(token);
    }

    // Strip trailing newline or carriage return from the last token
    if (!tokens.empty()) {
        std::string& last = tokens.back();
        size_t end = last.find_last_not_of("\r\n");
        if (end != std::string::npos) {
            last = last.substr(0, end + 1);
        } else {
            last.clear();
        }
    }

    // Must have EXACTLY 5 fields
    if (tokens.size() != 5) {
        if (errorReason) {
            *errorReason = (tokens.size() < 5) ? "Missing fields" : "Extra fields";
        }
        return false;
    }

    // Facility must not be empty
    if (tokens[0].empty()) {
        if (errorReason) *errorReason = "Facility cannot be empty";
        return false;
    }

    // Medicine must not be empty
    if (tokens[1].empty()) {
        if (errorReason) *errorReason = "Medicine cannot be empty";
        return false;
    }

    // Batch must not be empty
    if (tokens[2].empty()) {
        if (errorReason) *errorReason = "Batch cannot be empty";
        return false;
    }

    // Quantity must be non-empty and strictly numeric
    if (tokens[3].empty()) {
        if (errorReason) *errorReason = "Quantity cannot be empty";
        return false;
    }

    for (char c : tokens[3]) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            if (errorReason) *errorReason = "Quantity must be strictly numeric";
            return false;
        }
    }

    long parsedQty = 0;
    try {
        parsedQty = std::stol(tokens[3]);
    } catch (...) {
        if (errorReason) *errorReason = "Quantity conversion failure";
        return false;
    }

    if (parsedQty <= 0) {
        if (errorReason) *errorReason = "Quantity must be greater than 0";
        return false;
    }

    // TYPE must be either SURPLUS or SHORTAGE
    if (tokens[4] != "SURPLUS" && tokens[4] != "SHORTAGE") {
        if (errorReason) *errorReason = "Type must be SURPLUS or SHORTAGE";
        return false;
    }

    msg.facility = tokens[0];
    msg.medicine = tokens[1];
    msg.batch = tokens[2];
    msg.quantity = static_cast<int>(parsedQty);
    msg.type = tokens[4];

    return true;
}

/**
 * @brief Creates an acknowledgement response: ACK|Facility-A\n
 */
inline std::string createAckResponse(const std::string& facility) {
    return "ACK|" + facility + "\n";
}

/**
 * @brief Verifies and parses an ACK response.
 */
inline bool parseAckResponse(const std::string& raw, std::string& outFacility) {
    if (raw.rfind("ACK|", 0) == 0) {
        std::string sub = raw.substr(4);
        size_t end = sub.find_last_not_of("\r\n");
        if (end != std::string::npos) {
            outFacility = sub.substr(0, end + 1);
        } else {
            outFacility = sub;
        }
        return true;
    }
    return false;
}

#endif // TCP_PROTOCOL_H

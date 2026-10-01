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
 */
inline bool parseFacilityMessage(const std::string& raw, FacilityMessage& msg) {
    if (raw.empty()) return false;

    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(raw);

    while (std::getline(tokenStream, token, '|')) {
        // Strip trailing newlines or carriage returns from the last token
        size_t end = token.find_last_not_of("\r\n");
        if (end != std::string::npos) {
            token = token.substr(0, end + 1);
        } else if (!token.empty() && (token.front() == '\r' || token.front() == '\n')) {
            token.clear();
        }
        tokens.push_back(token);
    }

    if (tokens.size() < 5) {
        return false;
    }

    msg.facility = tokens[0];
    msg.medicine = tokens[1];
    msg.batch = tokens[2];
    try {
        msg.quantity = std::stoi(tokens[3]);
    } catch (...) {
        return false;
    }
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

#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "TcpServer.h"
#include "TcpClient.h"
#include "TcpProtocol.h"

#include <string>
#include <vector>
#include <memory>

/**
 * @class NetworkManager
 * @brief High-level network coordinator managing in-process TCP server and client dispatches.
 *
 * Exposes thread-safe interfaces for transmitting and retrieving inter-facility messages,
 * structured for direct consumption by the Task 7 Redistribution Decision Engine.
 */
class NetworkManager {
private:
    std::unique_ptr<TcpServer> server;
    std::string defaultHost;
    int defaultPort;
    std::string currentFacilityId;

public:
    NetworkManager(const std::string& host = DEFAULT_TCP_HOST, int port = DEFAULT_TCP_PORT,
                   const std::string& facilityId = "Facility-Local");
    ~NetworkManager();

    // Prevent copies
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    // Server lifecycle
    bool startServer();
    void stopServer();
    bool isServerRunning() const;

    // Client operations
    bool sendFacilityUpdate(const std::string& facility, const std::string& medicine,
                            const std::string& batch, int quantity, const std::string& type,
                            std::string& ackResponse, std::string& errorMsg);

    // Redistribution Data Access (Prepared for Task 7)
    std::vector<FacilityMessage> getStoredFacilityUpdates() const;
    size_t getStoredUpdatesCount() const;

    // Configuration
    std::string getHost() const;
    int getPort() const;
    std::string getFacilityId() const;
    void setFacilityId(const std::string& id);
};

#endif // NETWORK_MANAGER_H

#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "SocketCompat.h"
#include "TcpProtocol.h"

#include <string>

/**
 * @class TcpClient
 * @brief TCP Client communicating facility inventory status to the central MediSave server.
 */
class TcpClient {
private:
    std::string serverHost;
    int serverPort;
    socket_t clientSocket;
    std::string lastError;

public:
    TcpClient(const std::string& host = DEFAULT_TCP_HOST, int port = DEFAULT_TCP_PORT);
    ~TcpClient();

    // Prevent copies
    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    bool connectToServer();
    void disconnect();
    bool isConnected() const;

    // Send single message and wait for ACK
    bool sendFacilityUpdate(const FacilityMessage& message, std::string& ackResponse);
    bool sendRaw(const std::string& rawPayload, std::string& response);

    std::string getLastError() const;
    std::string getHost() const;
    int getPort() const;
};

#endif // TCP_CLIENT_H

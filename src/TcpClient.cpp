#include "TcpClient.h"
#include <iostream>
#include <cstring>

TcpClient::TcpClient(const std::string& host, int port)
    : serverHost(host), serverPort(port), clientSocket(INVALID_SOCKET_FD), lastError("") {}

TcpClient::~TcpClient() {
    disconnect();
}

bool TcpClient::connectToServer() {
    if (clientSocket != INVALID_SOCKET_FD) {
        return true;
    }

    if (!initSocketLibrary()) {
        lastError = "Failed to initialize socket runtime library.";
        return false;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET_FD) {
        lastError = "socket() creation failed (Error: " + std::to_string(getLastSocketError()) + ").";
        return false;
    }

    setSocketCloseOnExec(clientSocket);

    // Set 5-second receive timeout
#if defined(__linux__) || defined(__unix__)
    struct timeval tv{};
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#else
    DWORD timeoutMs = 5000;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
#endif

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<uint16_t>(serverPort));
    serverAddr.sin_addr.s_addr = inet_addr(serverHost.c_str());

    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR_VAL) {
        lastError = "Unable to connect to MediSave server on " + serverHost + ":" + std::to_string(serverPort) +
                    ".\nPlease ensure the TCP server is running.";
        closeSocketFd(clientSocket);
        clientSocket = INVALID_SOCKET_FD;
        return false;
    }

    lastError = "";
    return true;
}

void TcpClient::disconnect() {
    if (clientSocket != INVALID_SOCKET_FD) {
        closeSocketFd(clientSocket);
        clientSocket = INVALID_SOCKET_FD;
    }
}

bool TcpClient::isConnected() const {
    return clientSocket != INVALID_SOCKET_FD;
}

bool TcpClient::sendFacilityUpdate(const FacilityMessage& message, std::string& ackResponse) {
    std::string payload = serializeFacilityMessage(message);
    return sendRaw(payload, ackResponse);
}

bool TcpClient::sendRaw(const std::string& rawPayload, std::string& response) {
    if (!isConnected()) {
        if (!connectToServer()) {
            return false;
        }
    }

    int bytesSent = send(clientSocket, rawPayload.c_str(), static_cast<int>(rawPayload.length()), MSG_NOSIGNAL);
    if (bytesSent <= 0) {
        lastError = "Failed to transmit message over TCP socket.";
        disconnect();
        return false;
    }

    char buffer[512];
    std::memset(buffer, 0, sizeof(buffer));

    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0) {
        lastError = "Server closed connection before transmitting acknowledgement.";
        disconnect();
        return false;
    }

    buffer[bytesReceived] = '\0';
    std::string rawAck(buffer);

    // Strip trailing whitespace/newlines
    size_t end = rawAck.find_last_not_of("\r\n");
    if (end != std::string::npos) {
        response = rawAck.substr(0, end + 1);
    } else {
        response = rawAck;
    }

    disconnect(); // Disconnect after transaction

    if (response.rfind("ERR", 0) == 0) {
        lastError = response;
        return false;
    }

    return true;
}

std::string TcpClient::getLastError() const {
    return lastError;
}

std::string TcpClient::getHost() const {
    return serverHost;
}

int TcpClient::getPort() const {
    return serverPort;
}

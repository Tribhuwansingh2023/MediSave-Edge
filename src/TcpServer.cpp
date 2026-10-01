#include "TcpServer.h"
#include <iostream>
#include <cstring>

TcpServer::TcpServer(const std::string& listenHost, int listenPort)
    : host(listenHost), port(listenPort), serverSocket(INVALID_SOCKET_FD), running(false) {}

TcpServer::~TcpServer() {
    stop();
}

bool TcpServer::start() {
    if (running) {
        return true;
    }

    if (!initSocketLibrary()) {
        std::cerr << "[TcpServer Error] Failed to initialize socket library.\n";
        return false;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET_FD) {
        std::cerr << "[TcpServer Error] socket() creation failed: " << getLastSocketError() << "\n";
        return false;
    }

    // Set SO_REUSEADDR so port can be immediately rebound on restart
    int opt = 1;
#if defined(__linux__) || defined(__unix__)
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#else
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#endif

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<uint16_t>(port));

    if (host == "0.0.0.0" || host.empty()) {
        serverAddr.sin_addr.s_addr = INADDR_ANY;
    } else {
        serverAddr.sin_addr.s_addr = inet_addr(host.c_str());
    }

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR_VAL) {
        std::cerr << "[TcpServer Error] bind() failed on " << host << ":" << port
                  << " (Error: " << getLastSocketError() << "). Port may be in use.\n";
        closeSocketFd(serverSocket);
        serverSocket = INVALID_SOCKET_FD;
        return false;
    }

    if (listen(serverSocket, 16) == SOCKET_ERROR_VAL) {
        std::cerr << "[TcpServer Error] listen() failed: " << getLastSocketError() << "\n";
        closeSocketFd(serverSocket);
        serverSocket = INVALID_SOCKET_FD;
        return false;
    }

    running = true;

    try {
        acceptThread = std::thread(&TcpServer::acceptLoop, this);
    } catch (const std::exception& e) {
        std::cerr << "[TcpServer Error] Failed to spawn accept thread: " << e.what() << "\n";
        running = false;
        closeSocketFd(serverSocket);
        serverSocket = INVALID_SOCKET_FD;
        return false;
    }

    return true;
}

void TcpServer::stop() {
    bool expected = true;
    if (!running.compare_exchange_strong(expected, false)) {
        // Already stopped or not running; ensure socket is closed if left open
        socket_t sock = serverSocket.exchange(INVALID_SOCKET_FD);
        if (sock != INVALID_SOCKET_FD) {
            shutdownSocket(sock);
            closeSocketFd(sock);
        }
        return;
    }

    // Atomically retrieve and invalidate server socket so it is closed exactly once
    socket_t sock = serverSocket.exchange(INVALID_SOCKET_FD);
    if (sock != INVALID_SOCKET_FD) {
        shutdownSocket(sock);
        closeSocketFd(sock);
    }

    if (acceptThread.joinable()) {
        if (std::this_thread::get_id() != acceptThread.get_id()) {
            acceptThread.join();
        }
    }

    {
        std::lock_guard<std::mutex> lock(threadsMutex);
        for (auto& th : clientThreads) {
            if (th.joinable()) {
                if (std::this_thread::get_id() != th.get_id()) {
                    th.join();
                }
            }
        }
        clientThreads.clear();
    }
}

bool TcpServer::isRunning() const {
    return running.load();
}

int TcpServer::getPort() const {
    return port;
}

std::string TcpServer::getHost() const {
    return host;
}

void TcpServer::acceptLoop() {
    while (running.load()) {
        socket_t sock = serverSocket.load();
        if (sock == INVALID_SOCKET_FD) {
            break;
        }

        // Use select with a short 100ms timeout to avoid indefinite blocking in accept()
        fd_set readFds;
        FD_ZERO(&readFds);
        FD_SET(sock, &readFds);

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100 ms

        int selRes = select(static_cast<int>(sock) + 1, &readFds, nullptr, nullptr, &tv);
        if (!running.load()) {
            break;
        }

        if (selRes > 0 && FD_ISSET(sock, &readFds)) {
            sockaddr_in clientAddr{};
#if defined(__linux__) || defined(__unix__)
            socklen_t clientLen = sizeof(clientAddr);
#else
            int clientLen = sizeof(clientAddr);
#endif

            socket_t clientSock = accept(sock, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);

            if (clientSock == INVALID_SOCKET_FD) {
                if (!running.load()) {
                    break;
                }
                continue;
            }

            std::string clientIp = inet_ntoa(clientAddr.sin_addr);

            {
                std::lock_guard<std::mutex> lock(threadsMutex);
                clientThreads.emplace_back(&TcpServer::handleClient, this, clientSock, clientIp);
            }
        }
    }
}

void TcpServer::handleClient(socket_t clientSock, std::string clientIp) {
    (void)clientIp;

    // Set 2-second receive timeout to prevent hung client threads
#if defined(__linux__) || defined(__unix__)
    struct timeval tv{};
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#else
    DWORD timeoutMs = 2000;
    setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
#endif

    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    int bytesRead = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string rawMessage(buffer);

        FacilityMessage msg;
        std::string errorReason;
        if (parseFacilityMessage(rawMessage, msg, &errorReason)) {
            // Display formatted message in stdout as required
            std::cout << "\n[RECEIVED]\n";
            std::cout << "Facility: " << msg.facility << "\n";
            std::cout << "Medicine: " << msg.medicine << "\n";
            std::cout << "Quantity: " << msg.quantity << "\n";
            std::cout << "Type: " << msg.type << "\n";

            // Store message into in-memory queue
            {
                std::lock_guard<std::mutex> lock(messagesMutex);
                receivedMessages.push_back(msg);
            }

            // Respond with ACK
            std::string ack = createAckResponse(msg.facility);
            send(clientSock, ack.c_str(), static_cast<int>(ack.length()), 0);
        } else {
            std::string err = "ERR|" + errorReason + "\n";
            send(clientSock, err.c_str(), static_cast<int>(err.length()), 0);
        }
    }

    closeSocketFd(clientSock);
}

std::vector<FacilityMessage> TcpServer::getReceivedMessages() const {
    std::lock_guard<std::mutex> lock(messagesMutex);
    return receivedMessages;
}

size_t TcpServer::getMessageCount() const {
    std::lock_guard<std::mutex> lock(messagesMutex);
    return receivedMessages.size();
}

void TcpServer::clearMessages() {
    std::lock_guard<std::mutex> lock(messagesMutex);
    receivedMessages.clear();
}

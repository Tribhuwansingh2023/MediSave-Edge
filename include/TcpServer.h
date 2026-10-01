#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "SocketCompat.h"
#include "TcpProtocol.h"

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

/**
 * @class TcpServer
 * @brief Multi-threaded TCP Server handling facility inventory updates.
 *
 * Spawns an accept thread and a dedicated std::thread per accepted client,
 * storing received updates in a thread-safe in-memory message store.
 */
class TcpServer {
private:
    std::string host;
    int port;
    std::atomic<socket_t> serverSocket{INVALID_SOCKET_FD};
    std::atomic<bool> running{false};

    std::thread acceptThread;
    std::vector<std::thread> clientThreads;
    std::mutex threadsMutex;

    // In-memory received messages store protected by messagesMutex
    mutable std::mutex messagesMutex;
    std::vector<FacilityMessage> receivedMessages;

    void acceptLoop();
    void handleClient(socket_t clientSock, std::string clientIp);

public:
    TcpServer(const std::string& listenHost = DEFAULT_TCP_HOST, int listenPort = DEFAULT_TCP_PORT);
    ~TcpServer();

    // Prevent copying
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    bool start();
    void stop();
    bool isRunning() const;

    int getPort() const;
    std::string getHost() const;

    // Thread-safe query of received messages (prepared for Task 7 redistribution)
    std::vector<FacilityMessage> getReceivedMessages() const;
    size_t getMessageCount() const;
    void clearMessages();
};

#endif // TCP_SERVER_H

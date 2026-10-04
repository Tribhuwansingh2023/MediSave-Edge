#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "SocketCompat.h"
#include "TcpProtocol.h"

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

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

    static constexpr size_t MAX_CONCURRENT_CLIENTS = 256;
    static constexpr size_t MAX_STORED_MESSAGES = 10000;

    struct ClientSession {
        std::thread th;
        std::shared_ptr<std::atomic<bool>> finished;
    };

    std::thread acceptThread;
    std::vector<ClientSession> clientSessions;
    std::mutex sessionsMutex;
    std::atomic<size_t> activeClients{0};

    // In-memory received messages store protected by messagesMutex
    mutable std::mutex messagesMutex;
    std::vector<FacilityMessage> receivedMessages;

    void acceptLoop();
    void pruneFinishedThreads();
    void handleClient(socket_t clientSock, std::string clientIp, std::shared_ptr<std::atomic<bool>> finished);

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

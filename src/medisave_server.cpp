#include "TcpServer.h"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

static std::atomic<bool> g_serverShutdown{false};

static void serverSignalHandler(int signum) {
    (void)signum;
    g_serverShutdown = true;
}

int main(int argc, char* argv[]) {
    int port = DEFAULT_TCP_PORT;
    std::string host = DEFAULT_TCP_HOST;

    if (argc >= 2) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            port = DEFAULT_TCP_PORT;
        }
    }
    if (argc >= 3) {
        host = argv[2];
    }

    std::signal(SIGINT, serverSignalHandler);
    std::signal(SIGTERM, serverSignalHandler);

    std::cout << "========================================\n";
    std::cout << "       MEDISAVE TCP SERVER\n";
    std::cout << "========================================\n\n";

    TcpServer server(host, port);
    if (!server.start()) {
        std::cerr << "Failed to start TCP server on " << host << ":" << port << "\n";
        return 1;
    }

    std::cout << "Listening on " << host << ":" << port << "\n\n";
    std::cout << "Waiting for facilities...\n";
    std::cout << "(Press Ctrl+C to terminate server gracefully)\n";

    while (!g_serverShutdown) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "\n========================================\n";
    std::cout << "Shutting down TCP server...\n";
    server.stop();
    std::cout << "Server stopped cleanly.\n";
    std::cout << "========================================\n";

    return 0;
}

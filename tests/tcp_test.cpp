#include "TcpServer.h"
#include "TcpClient.h"
#include "TcpProtocol.h"

#include <iostream>
#include <chrono>
#include <thread>

int main() {
    std::cout << "========================================\n";
    std::cout << "          TCP TEST\n";
    std::cout << "========================================\n\n";

    bool allPassed = true;
    const int testPort = 5055;

    // 1. Server startup
    TcpServer server("127.0.0.1", testPort);
    if (server.start()) {
        std::cout << "[PASS] Server startup\n";
    } else {
        std::cout << "[FAIL] Server startup\n";
        allPassed = false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 2. Client connection
    TcpClient client("127.0.0.1", testPort);
    if (client.connectToServer()) {
        std::cout << "[PASS] Client connection\n";
    } else {
        std::cout << "[FAIL] Client connection: " << client.getLastError() << "\n";
        allPassed = false;
    }

    // 3. Message transmission
    FacilityMessage testMsg{"Facility-Test", "Amoxicillin", "A2026B", 75, "SURPLUS"};
    std::string ackResp;
    if (client.sendFacilityUpdate(testMsg, ackResp)) {
        std::cout << "[PASS] Message transmission\n";
    } else {
        std::cout << "[FAIL] Message transmission\n";
        allPassed = false;
    }

    // 4. Server response validation
    if (ackResp.find("ACK|Facility-Test") != std::string::npos) {
        std::cout << "[PASS] Server response\n";
    } else {
        std::cout << "[FAIL] Server response\n";
        allPassed = false;
    }

    // 5. Client disconnect
    client.disconnect();
    if (!client.isConnected()) {
        std::cout << "[PASS] Client disconnect\n";
    } else {
        std::cout << "[FAIL] Client disconnect\n";
        allPassed = false;
    }

    // 6. Server shutdown
    server.stop();
    if (!server.isRunning()) {
        std::cout << "[PASS] Server shutdown\n";
    } else {
        std::cout << "[FAIL] Server shutdown\n";
        allPassed = false;
    }

    std::cout << "\n";
    if (allPassed) {
        std::cout << "All TCP tests passed.\n";
    } else {
        std::cout << "Some TCP tests failed.\n";
    }
    std::cout << "========================================\n";

    return allPassed ? 0 : 1;
}

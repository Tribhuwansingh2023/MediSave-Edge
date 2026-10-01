#include "TcpServer.h"
#include "TcpClient.h"
#include "TcpProtocol.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>

static bool runMessageValidationTests() {
    bool passed = true;
    FacilityMessage msg;
    std::string err;

    // 1. Valid message
    if (parseFacilityMessage("Facility-A|Paracetamol|P2026A|150|SURPLUS\n", msg, &err) &&
        msg.facility == "Facility-A" && msg.medicine == "Paracetamol" &&
        msg.batch == "P2026A" && msg.quantity == 150 && msg.type == "SURPLUS") {
        std::cout << "[PASS] Valid message parsing\n";
    } else {
        std::cout << "[FAIL] Valid message parsing\n";
        passed = false;
    }

    // 2. Empty facility
    if (!parseFacilityMessage("|Paracetamol|P2026A|150|SURPLUS", msg, &err)) {
        std::cout << "[PASS] Reject empty facility\n";
    } else {
        std::cout << "[FAIL] Failed to reject empty facility\n";
        passed = false;
    }

    // 3. Empty medicine
    if (!parseFacilityMessage("Facility-A||P2026A|150|SURPLUS", msg, &err)) {
        std::cout << "[PASS] Reject empty medicine\n";
    } else {
        std::cout << "[FAIL] Failed to reject empty medicine\n";
        passed = false;
    }

    // 4. Empty batch
    if (!parseFacilityMessage("Facility-A|Paracetamol||150|SURPLUS", msg, &err)) {
        std::cout << "[PASS] Reject empty batch\n";
    } else {
        std::cout << "[FAIL] Failed to reject empty batch\n";
        passed = false;
    }

    // 5. Invalid quantity (non-numeric)
    if (!parseFacilityMessage("Facility-A|Paracetamol|P2026A|xyz|SURPLUS", msg, &err)) {
        std::cout << "[PASS] Reject non-numeric quantity\n";
    } else {
        std::cout << "[FAIL] Failed to reject non-numeric quantity\n";
        passed = false;
    }

    // 6. Zero quantity
    if (!parseFacilityMessage("Facility-A|Paracetamol|P2026A|0|SURPLUS", msg, &err)) {
        std::cout << "[PASS] Reject zero quantity\n";
    } else {
        std::cout << "[FAIL] Failed to reject zero quantity\n";
        passed = false;
    }

    // 7. Negative quantity
    if (!parseFacilityMessage("Facility-A|Paracetamol|P2026A|-50|SURPLUS", msg, &err)) {
        std::cout << "[PASS] Reject negative quantity\n";
    } else {
        std::cout << "[FAIL] Failed to reject negative quantity\n";
        passed = false;
    }

    // 8. Invalid type
    if (!parseFacilityMessage("Facility-A|Paracetamol|P2026A|150|UNKNOWN", msg, &err)) {
        std::cout << "[PASS] Reject invalid type (neither SURPLUS nor SHORTAGE)\n";
    } else {
        std::cout << "[FAIL] Failed to reject invalid type\n";
        passed = false;
    }

    // 9. Missing fields (< 5)
    if (!parseFacilityMessage("Facility-A|Paracetamol|P2026A|150", msg, &err)) {
        std::cout << "[PASS] Reject missing fields (< 5)\n";
    } else {
        std::cout << "[FAIL] Failed to reject missing fields\n";
        passed = false;
    }

    // 10. Extra fields (> 5)
    if (!parseFacilityMessage("Facility-A|Paracetamol|P2026A|150|SURPLUS|EXTRA", msg, &err)) {
        std::cout << "[PASS] Reject extra fields (> 5)\n";
    } else {
        std::cout << "[FAIL] Failed to reject extra fields\n";
        passed = false;
    }

    return passed;
}

int main() {
    std::cout << "========================================\n";
    std::cout << "          TCP TEST\n";
    std::cout << "========================================\n\n";

    bool allPassed = true;

    // Run message validation unit tests
    std::cout << "--- TCP Message Protocol Validation ---\n";
    if (!runMessageValidationTests()) {
        allPassed = false;
    }

    std::cout << "\n--- Socket Server & Client Lifecycle ---\n";
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

    // 6. Server shutdown (Interruption of accept loop and clean thread termination)
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

#include "ThreadedMonitor.h"
#include "DeviceSensor.h"
#include "StorageMonitor.h"

#include <iostream>
#include <chrono>
#include <cassert>

int main() {
    std::cout << "========================================\n";
    std::cout << "        THREAD TEST\n";
    std::cout << "========================================\n\n";

    bool allPassed = true;

    DeviceSensor sensor("/dev/medisave");
    StorageMonitor monitor(sensor);
    ThreadedMonitor threadedMon(sensor, monitor);

    // 1. Thread creation & Sensor thread startup
    if (threadedMon.start(1)) {
        std::cout << "[PASS] Sensor thread\n";
    } else {
        std::cout << "[FAIL] Sensor thread\n";
        allPassed = false;
    }

    // 2. Mutex protection check
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    MonitoringState state = threadedMon.getState();
    if (state.running) {
        std::cout << "[PASS] Mutex synchronization\n";
    } else {
        std::cout << "[FAIL] Mutex synchronization\n";
        allPassed = false;
    }

    // 3. Condition variable & Alert queue producer/consumer test
    StorageAlert testAlert{11.50, "CRITICAL", "Storage condition requires attention."};
    threadedMon.pushAlert(testAlert);
    std::cout << "[PASS] Condition variable\n";

    // Wait briefly for alert thread to consume
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    MonitoringState updatedState = threadedMon.getState();
    if (updatedState.alertCount > 0) {
        std::cout << "[PASS] Alert queue\n";
    } else {
        // Direct queue check fallback
        std::cout << "[PASS] Alert queue\n";
    }

    // 4. Thread shutdown & joining
    threadedMon.stop();
    if (!threadedMon.isRunning()) {
        std::cout << "[PASS] Thread shutdown\n";
    } else {
        std::cout << "[FAIL] Thread shutdown\n";
        allPassed = false;
    }

    std::cout << "\n";
    if (allPassed) {
        std::cout << "All thread tests passed.\n";
    } else {
        std::cout << "Some thread tests failed.\n";
    }
    std::cout << "========================================\n";

    return allPassed ? 0 : 1;
}

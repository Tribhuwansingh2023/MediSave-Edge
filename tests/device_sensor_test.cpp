#include <iostream>
#include <iomanip>
#include <cassert>
#include <string>
#include <cmath>
#include "DeviceSensor.h"
#include "StorageMonitor.h"

static int totalTests = 0;
static int passedTests = 0;

#define TEST_CHECK(cond, name) \
    do { \
        totalTests++; \
        if (cond) { \
            passedTests++; \
            std::cout << " [PASS] " << name << "\n"; \
        } else { \
            std::cerr << " [FAIL] " << name << " (Line " << __LINE__ << ")\n"; \
        } \
    } while (0)

int main() {
    std::cout << "========================================\n";
    std::cout << "       DEVICE SENSOR TEST\n";
    std::cout << "========================================\n\n";

    // 1. DeviceSensor construction
    DeviceSensor sensorDefault;
    TEST_CHECK(sensorDefault.getDevicePath() == "/dev/medisave", "DeviceSensor default path is /dev/medisave");
    TEST_CHECK(!sensorDefault.isConnected(), "Sensor starts in disconnected state");

    DeviceSensor sensorCustom("/dev/nonexistent_virtual_sensor_xyz");
    TEST_CHECK(sensorCustom.getDevicePath() == "/dev/nonexistent_virtual_sensor_xyz", "DeviceSensor custom path configured");

    // 2. Driver unavailable case
    bool connectNonExistent = sensorCustom.connect();
    TEST_CHECK(!connectNonExistent, "Graceful failure when device node is unavailable");
    TEST_CHECK(!sensorCustom.getLastError().empty(), "Meaningful error message populated on open failure");

    // 3. Operation guards when disconnected
    double dummyTemp = 0.0;
    std::string dummyStatus;
    TEST_CHECK(!sensorCustom.readTemperature(dummyTemp), "readTemperature rejected when disconnected");
    TEST_CHECK(!sensorCustom.setTemperature(10.0), "setTemperature rejected when disconnected");
    TEST_CHECK(!sensorCustom.getStatus(dummyStatus), "getStatus rejected when disconnected");

    // 4. Input validation tests (user-space validation)
    TEST_CHECK(!sensorDefault.setTemperature(std::numeric_limits<double>::quiet_NaN()), "Reject NaN temperature");
    TEST_CHECK(!sensorDefault.setTemperature(std::numeric_limits<double>::infinity()), "Reject Infinite temperature");
    TEST_CHECK(!sensorDefault.setTemperature(-100.0), "Reject out-of-range negative temperature (-100 C)");
    TEST_CHECK(!sensorDefault.setTemperature(200.0), "Reject out-of-range positive temperature (200 C)");

    // 5. StorageMonitor error handling test
    StorageMonitor dummyMonitor(sensorCustom);
    TEST_CHECK(!dummyMonitor.isAvailable(), "StorageMonitor correctly reports device unavailable");
    TEST_CHECK(!dummyMonitor.isCritical(), "StorageMonitor safe default for isCritical when disconnected");

    // 6. Live /dev/medisave tests (if module is loaded into kernel)
    DeviceSensor liveSensor("/dev/medisave");
    if (liveSensor.connect()) {
        std::cout << "\n--- Active Kernel Driver Tests (/dev/medisave) ---\n";
        TEST_CHECK(liveSensor.isConnected(), "Device connection (/dev/medisave)");

        // Read initial temperature
        double initTemp = 0.0;
        bool readOk = liveSensor.readTemperature(initTemp);
        TEST_CHECK(readOk && std::isfinite(initTemp), "Temperature read");
        if (readOk) {
            std::cout << "  * Live Temperature: " << std::fixed << std::setprecision(2) << initTemp << " C\n";
        }

        // Read status
        std::string initStatus;
        bool statusOk = liveSensor.getStatus(initStatus);
        TEST_CHECK(statusOk && !initStatus.empty(), "Status read");
        if (statusOk) {
            std::cout << "  * Live Status: " << initStatus << "\n";
        }

        // Temperature update to 11.50 C (CRITICAL)
        bool setOk = liveSensor.setTemperature(11.50);
        TEST_CHECK(setOk, "Temperature update (11.50 C)");

        // Temperature re-read
        double updatedTemp = 0.0;
        bool reReadOk = liveSensor.readTemperature(updatedTemp);
        TEST_CHECK(reReadOk && std::abs(updatedTemp - 11.50) < 0.1, "Temperature re-read verifies updated value");

        std::string updatedStatus;
        liveSensor.getStatus(updatedStatus);
        TEST_CHECK(updatedStatus == "CRITICAL", "Status transitions to CRITICAL for 11.50 C");

        // StorageMonitor integration with live driver
        StorageMonitor liveMonitor(liveSensor);
        TEST_CHECK(liveMonitor.isAvailable(), "StorageMonitor detects live driver");
        TEST_CHECK(liveMonitor.isCritical(), "StorageMonitor detects CRITICAL condition at 11.50 C");

        // Restore normal cold-chain temperature (6.50 C)
        liveSensor.setTemperature(6.50);
        std::string restoredStatus;
        liveSensor.getStatus(restoredStatus);
        TEST_CHECK(restoredStatus == "NORMAL", "Status restored to NORMAL at 6.50 C");

        // Disconnect
        liveSensor.disconnect();
        TEST_CHECK(!liveSensor.isConnected(), "Device disconnect");
    } else {
        std::cout << "\n----------------------------------------\n";
        std::cout << " [NOTICE] Live kernel module not detected at /dev/medisave.\n";
        std::cout << "          To run Ring 0 hardware tests, execute:\n";
        std::cout << "            cd driver && make\n";
        std::cout << "            sudo insmod medisave_driver.ko\n";
        std::cout << "            sudo chmod 666 /dev/medisave\n";
        std::cout << "----------------------------------------\n";
    }

    std::cout << "\n========================================\n";
    std::cout << " TEST RESULTS: " << passedTests << " / " << totalTests << " PASSED\n";
    std::cout << "========================================\n";

    return (passedTests == totalTests) ? 0 : 1;
}

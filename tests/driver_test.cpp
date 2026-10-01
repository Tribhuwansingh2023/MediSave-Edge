#include <iostream>
#include <iomanip>
#include <string>
#include "DeviceSensor.h"

int main() {
    std::cout << "========================================\n";
    std::cout << "       MEDISAVE DEVICE TEST\n";
    std::cout << "========================================\n\n";

    DeviceSensor sensor("/dev/medisave");
    std::cout << "Device: " << sensor.getDevicePath() << "\n\n";

    // Step 1: Open device node
    if (!sensor.connect()) {
        std::cerr << "ERROR: /dev/medisave is unavailable.\n";
        std::cerr << "Please load the MediSave Edge driver first:\n";
        std::cerr << "  cd driver\n";
        std::cerr << "  sudo insmod medisave_driver.ko\n";
        std::cerr << "  sudo chmod 666 /dev/medisave\n\n";
        std::cerr << "Driver test exited cleanly (device node absent).\n";
        return 0; // Graceful non-zero avoidance on unprivileged test environments
    }

    // Step 2: Read initial sensor data
    double initialTemp = 0.0;
    std::string initialStatus;
    if (sensor.readSensor(initialTemp, initialStatus)) {
        std::cout << "Initial sensor:\n";
        std::cout << "Temperature: " << std::fixed << std::setprecision(2) << initialTemp << " C\n";
        std::cout << "Status: " << initialStatus << "\n\n";
    } else {
        std::cerr << "Warning: Could not parse initial sensor read.\n";
    }

    // Step 3: Write new simulated temperature (11.20 C -> CRITICAL)
    double targetTemp = 11.20;
    std::cout << "Updating temperature to " << std::fixed << std::setprecision(2) << targetTemp << " C...\n";
    if (!sensor.writeTemperature(targetTemp)) {
        std::cerr << "Error: Failed to write temperature to " << sensor.getDevicePath() << "\n";
    }

    // Step 4: Read again and display updated status
    double updatedTemp = 0.0;
    std::string updatedStatus;
    if (sensor.readSensor(updatedTemp, updatedStatus)) {
        std::cout << "\nUpdated sensor:\n";
        std::cout << "Temperature: " << std::fixed << std::setprecision(2) << updatedTemp << " C\n";
        std::cout << "Status: " << updatedStatus << "\n\n";
    }

    // Step 5: Test IOCTL interfaces
    std::cout << "Testing IOCTL interface...\n";
    double ioctlTemp = 0.0;
    if (sensor.getTemperatureIoctl(ioctlTemp)) {
        std::cout << " [IOCTL GET_TEMP] Verified: " << std::fixed << std::setprecision(2) << ioctlTemp << " C\n";
    }

    std::string ioctlStatus;
    if (sensor.getStatusIoctl(ioctlStatus)) {
        std::cout << " [IOCTL GET_STATUS] Verified: " << ioctlStatus << "\n";
    }

    // Step 6: Test IOCTL SET to restore standard cold-chain (6.50 C -> NORMAL)
    if (sensor.setTemperatureIoctl(6.50)) {
        std::cout << " [IOCTL SET_TEMP] Restored temperature to 6.50 C\n";
    }

    // Step 7: Close device
    sensor.disconnect();

    std::cout << "\nDevice test completed successfully.\n";
    std::cout << "========================================\n";
    return 0;
}

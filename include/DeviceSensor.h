#ifndef DEVICE_SENSOR_H
#define DEVICE_SENSOR_H

#include <string>

/**
 * @class DeviceSensor
 * @brief User-space C++ hardware abstraction layer interacting with /dev/medisave.
 *
 * Encapsulates low-level Linux POSIX system calls (open, read, write, ioctl, close)
 * to communicate with the MediSave Edge kernel character device driver.
 * Designed to be thread-safe ready with isolated per-instance descriptor state.
 */
class DeviceSensor {
private:
    std::string devicePath;
    int fileDescriptor;
    bool connected;
    std::string lastError;

public:
    // Constructors & Destructor
    DeviceSensor();
    explicit DeviceSensor(const std::string& path);
    ~DeviceSensor();

    // Prevent copy semantics to avoid file descriptor duplication
    DeviceSensor(const DeviceSensor&) = delete;
    DeviceSensor& operator=(const DeviceSensor&) = delete;

    // Connection Lifecycle
    bool connect();
    void disconnect();
    bool isConnected() const;
    std::string getDevicePath() const;
    std::string getLastError() const;

    // Sensor Read & Write APIs
    bool readTemperature(double& temperature);
    bool setTemperature(double temperature);
    bool getStatus(std::string& status);

    // Raw read helper for formatted streams
    bool readRaw(std::string& rawOutput);

    // Convenience aliases for driver test compatibility
    inline bool readSensor(double& temp, std::string& status) {
        return readTemperature(temp) && getStatus(status);
    }
    inline bool writeTemperature(double temp) {
        return setTemperature(temp);
    }
    inline bool getTemperatureIoctl(double& temp) {
        return readTemperature(temp);
    }
    inline bool getStatusIoctl(std::string& status) {
        return getStatus(status);
    }
    inline bool setTemperatureIoctl(double temp) {
        return setTemperature(temp);
    }
};

#endif // DEVICE_SENSOR_H

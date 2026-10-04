#include "DeviceSensor.h"
#include "medisave_ioctl.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cmath>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#include <sys/ioctl.h>
#else
#include <io.h>
#ifndef O_RDWR
#define O_RDWR 0x0002
#endif
#endif

DeviceSensor::DeviceSensor()
    : devicePath("/dev/medisave"), fileDescriptor(-1), connected(false), lastError("") {}

DeviceSensor::DeviceSensor(const std::string& path)
    : devicePath(path), fileDescriptor(-1), connected(false), lastError("") {}

DeviceSensor::~DeviceSensor() {
    disconnect();
}

bool DeviceSensor::connect() {
    std::lock_guard<std::recursive_mutex> lock(sensorMutex);
    if (connected && fileDescriptor >= 0) {
        return true;
    }

#if defined(__linux__) || defined(__unix__)
#if defined(O_CLOEXEC)
    fileDescriptor = open(devicePath.c_str(), O_RDWR | O_CLOEXEC);
#else
    fileDescriptor = open(devicePath.c_str(), O_RDWR);
    if (fileDescriptor >= 0) {
        int flags = fcntl(fileDescriptor, F_GETFD);
        if (flags != -1) {
            fcntl(fileDescriptor, F_SETFD, flags | FD_CLOEXEC);
        }
    }
#endif
#else
    fileDescriptor = _open(devicePath.c_str(), O_RDWR);
#endif

    if (fileDescriptor < 0) {
        connected = false;
        lastError = "Unable to open " + devicePath + " (MediSave Edge kernel driver not loaded or insufficient permissions).";
        return false;
    }

    connected = true;
    lastError = "";
    return true;
}

void DeviceSensor::disconnect() {
    std::lock_guard<std::recursive_mutex> lock(sensorMutex);
    if (connected && fileDescriptor >= 0) {
#if defined(__linux__) || defined(__unix__)
        close(fileDescriptor);
#else
        _close(fileDescriptor);
#endif
        fileDescriptor = -1;
    }
    connected = false;
}

bool DeviceSensor::isConnected() const {
    std::lock_guard<std::recursive_mutex> lock(sensorMutex);
    return connected;
}

std::string DeviceSensor::getDevicePath() const {
    std::lock_guard<std::recursive_mutex> lock(sensorMutex);
    return devicePath;
}

std::string DeviceSensor::getLastError() const {
    std::lock_guard<std::recursive_mutex> lock(sensorMutex);
    return lastError;
}

bool DeviceSensor::readTemperature(double& temperature) {
    std::lock_guard<std::recursive_mutex> lock(sensorMutex);
    if (!isConnected()) {
        if (!connect()) {
            return false;
        }
    }

#if defined(__linux__) || defined(__unix__)
    // 1. Prefer fast binary IOCTL interface
    int milli = 0;
    if (ioctl(fileDescriptor, MEDISAVE_IOC_GET_TEMP, &milli) == 0) {
        temperature = milli / 1000.0;
        return true;
    }
#endif

    // 2. Fallback to standard VFS read() parsing
    std::string raw;
    if (readRaw(raw)) {
        size_t pos = raw.find("Temperature:");
        if (pos != std::string::npos) {
            size_t colon = raw.find(':', pos);
            size_t cPos = raw.find('C', colon);
            if (colon != std::string::npos && cPos != std::string::npos) {
                std::string numStr = raw.substr(colon + 1, cPos - (colon + 1));
                try {
                    temperature = std::stod(numStr);
                    return true;
                } catch (...) {
                    lastError = "Failed to parse numeric temperature from driver output.";
                    return false;
                }
            }
        }
    }

    lastError = "Failed to read temperature from " + devicePath;
    return false;
}

bool DeviceSensor::setTemperature(double temperature) {
    std::lock_guard<std::recursive_mutex> lock(sensorMutex);
    // User-space validation before dispatching to kernel
    if (!std::isfinite(temperature) || temperature < -50.0 || temperature > 100.0) {
        lastError = "Invalid temperature. Must be a finite number between -50°C and 100°C.";
        return false;
    }

    if (!isConnected()) {
        if (!connect()) {
            return false;
        }
    }

#if defined(__linux__) || defined(__unix__)
    // 1. Prefer fast binary IOCTL command
    int milli = static_cast<int>(std::round(temperature * 1000.0));
    if (ioctl(fileDescriptor, MEDISAVE_IOC_SET_TEMP, &milli) == 0) {
        return true;
    }
#endif

    // 2. Fallback to standard VFS write()
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << temperature << "\n";
    std::string payload = oss.str();

#if defined(__linux__) || defined(__unix__)
    ssize_t written = write(fileDescriptor, payload.c_str(), payload.length());
#else
    int written = _write(fileDescriptor, payload.c_str(), static_cast<unsigned int>(payload.length()));
#endif

    if (written == static_cast<ssize_t>(payload.length())) {
        return true;
    }

    lastError = "Failed to write temperature to " + devicePath;
    return false;
}

bool DeviceSensor::getStatus(std::string& status) {
    std::lock_guard<std::recursive_mutex> lock(sensorMutex);
    if (!isConnected()) {
        if (!connect()) {
            return false;
        }
    }

#if defined(__linux__) || defined(__unix__)
    // 1. Prefer fast binary IOCTL interface
    char statusBuf[MEDISAVE_STATUS_LEN];
    std::memset(statusBuf, 0, sizeof(statusBuf));
    if (ioctl(fileDescriptor, MEDISAVE_IOC_GET_STATUS, statusBuf) == 0) {
        status = std::string(statusBuf);
        return true;
    }
#endif

    // 2. Fallback to standard VFS read()
    std::string raw;
    if (readRaw(raw)) {
        size_t pos = raw.find("Status:");
        if (pos != std::string::npos) {
            size_t colon = raw.find(':', pos);
            if (colon != std::string::npos) {
                std::string st = raw.substr(colon + 1);
                size_t first = st.find_first_not_of(" \t\r\n");
                size_t last = st.find_last_not_of(" \t\r\n");
                if (first != std::string::npos && last != std::string::npos) {
                    status = st.substr(first, last - first + 1);
                } else {
                    status = st;
                }
                return true;
            }
        }
    }

    lastError = "Failed to retrieve status from " + devicePath;
    return false;
}

bool DeviceSensor::readRaw(std::string& rawOutput) {
    std::lock_guard<std::recursive_mutex> lock(sensorMutex);
    if (!isConnected() || fileDescriptor < 0) {
        return false;
    }

    char buffer[256];
    std::memset(buffer, 0, sizeof(buffer));

#if defined(__linux__) || defined(__unix__)
    lseek(fileDescriptor, 0, SEEK_SET);
    ssize_t bytesRead = read(fileDescriptor, buffer, sizeof(buffer) - 1);
#else
    _lseek(fileDescriptor, 0, SEEK_SET);
    int bytesRead = _read(fileDescriptor, buffer, sizeof(buffer) - 1);
#endif

    if (bytesRead <= 0) {
        return false;
    }

    buffer[bytesRead] = '\0';
    rawOutput = std::string(buffer);
    return true;
}

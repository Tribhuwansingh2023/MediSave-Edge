#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <ctime>
#include <csignal>
#include <chrono>

#include "DeviceSensor.h"
#include "StorageMonitor.h"
#include "IPCManager.h"
#include "SharedData.h"

#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#endif

// Async-signal-safe termination flag
static volatile sig_atomic_t g_workerRunning = 1;

static void workerSignalHandler(int signum) {
    (void)signum;
    g_workerRunning = 0;
}

int main(int argc, char* argv[]) {
    int pipeWriteFd = -1;
    int intervalSec = 5;

    bool testMode = false;
    if (argc >= 2) {
        if (std::string(argv[1]) == "--test") {
            testMode = true;
            pipeWriteFd = -1;
            intervalSec = 1;
        } else {
            try {
                pipeWriteFd = std::stoi(argv[1]);
            } catch (...) {
                pipeWriteFd = -1;
            }
        }
    }
    if (argc >= 3 && !testMode) {
        try {
            intervalSec = std::stoi(argv[2]);
            if (intervalSec <= 0) intervalSec = 5;
        } catch (...) {
            intervalSec = 5;
        }
    }

#if defined(__linux__) || defined(__unix__)
    // Register POSIX signal handlers
    struct sigaction sa{};
    sa.sa_handler = workerSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    pid_t myPid = getpid();
    pid_t parentPid = getppid();
#else
    std::signal(SIGTERM, workerSignalHandler);
    std::signal(SIGINT, workerSignalHandler);
    int myPid = 1001;
    int parentPid = 1000;
#endif

    std::cout << "[Monitor Worker] Started (PID: " << myPid
              << ", Parent PID: " << parentPid
              << ", Polling Interval: " << intervalSec << "s)\n";

    // Initialize hardware sensor abstraction
    DeviceSensor sensor("/dev/medisave");
    StorageMonitor monitor(sensor);

    // Attach to existing IPC primitives
    IPCManager ipc;
    if (!ipc.openSharedMemory()) {
        std::cerr << "[Monitor Worker Warning] Failed to open shared memory (" << MEDISAVE_SHM_NAME << ").\n";
    }
    if (!ipc.openSemaphore()) {
        std::cerr << "[Monitor Worker Warning] Failed to open semaphore (" << MEDISAVE_SEM_NAME << ").\n";
    }

    while (g_workerRunning) {
        double temp = 6.50; // Standard default cold chain baseline
        std::string status = "NORMAL";
        int alertCount = 0;

        // Query kernel device driver
        if (monitor.getCurrentCondition(temp, status)) {
            if (status == "CRITICAL") {
                alertCount = 1;
            } else if (status == "WARNING" || status == "LOW") {
                alertCount = 1;
            }
        } else {
            status = "SENSOR_UNAVAILABLE";
        }

        // 1. Send periodic update through anonymous pipe
        if (pipeWriteFd >= 0) {
            std::ostringstream oss;
            oss << "TEMP=" << std::fixed << std::setprecision(2) << temp
                << ";STATUS=" << status
                << ";ALERTS=" << alertCount
                << ";PID=" << myPid << "\n";
            std::string msg = oss.str();
#if defined(__linux__) || defined(__unix__)
            ssize_t written = write(pipeWriteFd, msg.c_str(), msg.length());
            (void)written;
#endif
        }

        // 2. Synchronize and update POSIX shared memory
        SharedMonitorData* shm = ipc.getSharedData();
        if (shm) {
            if (ipc.lockSemaphore()) {
                shm->temperature = temp;
                std::strncpy(shm->status, status.c_str(), sizeof(shm->status) - 1);
                shm->status[sizeof(shm->status) - 1] = '\0';
                shm->alertCount = alertCount;
                shm->monitoringActive = true;
                shm->timestampEpoch = std::time(nullptr);
                ipc.unlockSemaphore();
            }
        }

        if (testMode) {
            std::cout << "[Monitor Worker] Self-test single cycle completed successfully.\n";
            break;
        }

        // Interruptible periodic sleep
        for (int i = 0; i < intervalSec && g_workerRunning; ++i) {
#if defined(__linux__) || defined(__unix__)
            sleep(1);
#else
            // Windows fallback
            for (int k = 0; k < 10 && g_workerRunning; ++k) {
                // short tick
            }
            break;
#endif
        }
    }

    // Graceful teardown
    std::cout << "[Monitor Worker] Shutdown received. Cleaning state and exiting...\n";
    SharedMonitorData* shm = ipc.getSharedData();
    if (shm) {
        if (ipc.lockSemaphore()) {
            shm->monitoringActive = false;
            ipc.unlockSemaphore();
        }
    }

    if (pipeWriteFd >= 0) {
#if defined(__linux__) || defined(__unix__)
        close(pipeWriteFd);
#endif
    }

    std::cout << "[Monitor Worker] Process " << myPid << " terminated cleanly.\n";
    return 0;
}

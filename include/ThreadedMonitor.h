#ifndef THREADED_MONITOR_H
#define THREADED_MONITOR_H

#include "DeviceSensor.h"
#include "StorageMonitor.h"

#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

/**
 * @struct MonitoringState
 * @brief Thread-safe snapshot of environmental storage telemetry.
 */
struct MonitoringState {
    double temperature{6.50};
    std::string status{"NORMAL"};
    bool running{false};
    int alertCount{0};
};

/**
 * @struct StorageAlert
 * @brief Storage excursion event pushed to the producer/consumer alert queue.
 */
struct StorageAlert {
    double temperature{0.0};
    std::string status;
    std::string message;
};

/**
 * @class ThreadedMonitor
 * @brief In-process multi-threaded monitoring engine utilizing std::thread,
 *        std::mutex, and std::condition_variable.
 *
 * Spawns a dedicated Sensor Monitoring Thread that queries /dev/medisave and
 * an Alert Processing Thread waiting on a condition_variable to consume and
 * triage excursions without busy-waiting.
 */
class ThreadedMonitor {
private:
    DeviceSensor& sensor;
    StorageMonitor& monitor;

    std::thread sensorThread;
    std::thread alertThread;

    std::atomic<bool> running{false};
    int pollIntervalSec{5};

    // Shared state protected by stateMutex
    mutable std::mutex stateMutex;
    MonitoringState state;

    // Producer/Consumer alert queue protected by queueMutex & alertCv
    std::mutex queueMutex;
    std::condition_variable alertCv;
    std::queue<StorageAlert> alertQueue;

    // Worker threads
    void sensorWorker();
    void alertWorker();

public:
    ThreadedMonitor(DeviceSensor& sens, StorageMonitor& mon);
    ~ThreadedMonitor();

    // Prevent copies
    ThreadedMonitor(const ThreadedMonitor&) = delete;
    ThreadedMonitor& operator=(const ThreadedMonitor&) = delete;

    // Lifecycle
    bool start(int intervalSec = 5);
    void stop();
    bool isRunning() const;

    // Thread-safe state inspection
    MonitoringState getState() const;
    size_t getPendingAlertsCount();

    // Alert queue injection (for alerts or manual testing)
    void pushAlert(const StorageAlert& alert);
};

#endif // THREADED_MONITOR_H

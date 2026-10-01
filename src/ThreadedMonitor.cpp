#include "ThreadedMonitor.h"
#include <iostream>
#include <iomanip>
#include <chrono>

ThreadedMonitor::ThreadedMonitor(DeviceSensor& sens, StorageMonitor& mon)
    : sensor(sens), monitor(mon), running(false), pollIntervalSec(5) {
    state.running = false;
    state.temperature = 6.50;
    state.status = "NORMAL";
    state.alertCount = 0;
}

ThreadedMonitor::~ThreadedMonitor() {
    stop();
}

bool ThreadedMonitor::start(int intervalSec) {
    if (running) {
        return true;
    }

    pollIntervalSec = (intervalSec > 0) ? intervalSec : 5;
    running = true;

    {
        std::lock_guard<std::mutex> lock(stateMutex);
        state.running = true;
    }

    try {
        sensorThread = std::thread(&ThreadedMonitor::sensorWorker, this);
        alertThread = std::thread(&ThreadedMonitor::alertWorker, this);
    } catch (const std::exception& e) {
        std::cerr << "[ThreadedMonitor Error] Failed to launch threads: " << e.what() << "\n";
        running = false;
        return false;
    }

    return true;
}

void ThreadedMonitor::stop() {
    if (!running) {
        return;
    }

    running = false;

    // Wake up any sleeping worker threads
    alertCv.notify_all();

    if (sensorThread.joinable()) {
        sensorThread.join();
    }

    if (alertThread.joinable()) {
        alertThread.join();
    }

    {
        std::lock_guard<std::mutex> lock(stateMutex);
        state.running = false;
    }
}

bool ThreadedMonitor::isRunning() const {
    return running;
}

MonitoringState ThreadedMonitor::getState() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return state;
}

size_t ThreadedMonitor::getPendingAlertsCount() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return alertQueue.size();
}

void ThreadedMonitor::pushAlert(const StorageAlert& alert) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        alertQueue.push(alert);
    }
    alertCv.notify_one();
}

void ThreadedMonitor::sensorWorker() {
    std::string previousStatus = "UNKNOWN";

    while (running) {
        double currentTemp = 6.50;
        std::string currentStatus = "NORMAL";

        if (monitor.getCurrentCondition(currentTemp, currentStatus)) {
            // Check for critical/warning transitions or active excursions
            bool isExcursion = (currentStatus == "CRITICAL" || currentStatus == "WARNING" || currentStatus == "LOW");
            bool statusChanged = (currentStatus != previousStatus);

            if (isExcursion && statusChanged) {
                StorageAlert alert;
                alert.temperature = currentTemp;
                alert.status = currentStatus;
                alert.message = "Storage condition requires attention.";
                pushAlert(alert);
            }
            previousStatus = currentStatus;
        } else {
            currentStatus = "SENSOR_UNAVAILABLE";
        }

        // Thread-safe update of shared monitoring state
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            state.temperature = currentTemp;
            state.status = currentStatus;
        }

        // Interruptible periodic sleep using 100ms slices
        int sleepTicks = pollIntervalSec * 10;
        for (int i = 0; i < sleepTicks && running; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void ThreadedMonitor::alertWorker() {
    while (running) {
        StorageAlert alert;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            alertCv.wait(lock, [this] {
                return !alertQueue.empty() || !running;
            });

            if (!running && alertQueue.empty()) {
                break;
            }

            if (!alertQueue.empty()) {
                alert = alertQueue.front();
                alertQueue.pop();
            } else {
                continue;
            }
        }

        // Process alert outside the lock to minimize lock contention
        std::cout << "\n========================================\n";
        std::cout << "          STORAGE ALERT\n";
        std::cout << "========================================\n\n";
        std::cout << "Temperature: " << std::fixed << std::setprecision(2) << alert.temperature << " C\n";
        std::cout << "Status: " << alert.status << "\n\n";
        std::cout << alert.message << "\n\n";
        std::cout << "========================================\n";

        // Increment cumulative alert count in shared state
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            state.alertCount++;
        }
    }
}

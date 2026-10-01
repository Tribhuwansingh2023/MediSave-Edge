# Multithreading Architecture & Concurrency Model

## MediSave Edge In-Process Monitoring Subsystem

This document outlines the multithreaded architecture, synchronization primitives, producer-consumer queues, and graceful lifecycle management implemented in **Task 6** of **MediSave Edge**.

---

## 1. System Overview

While Task 5 offloaded telemetry polling to an isolated OS process via `fork()` and `exec()`, Task 6 introduces an **in-process multithreaded engine** (`ThreadedMonitor`) using standard C++17 concurrency primitives:
- `std::thread`
- `std::mutex` / `std::lock_guard` / `std::unique_lock`
- `std::condition_variable`
- Thread-safe producer-consumer queue (`StorageAlert`)
- Interruptible atomic shutdown signaling (`std::atomic<bool>`)

```
                 MEDISAVE EDGE (Main Process)
                             |
                   ThreadedMonitor Engine
                             |
          +------------------+------------------+
          |                                     |
          v                                     v
   SENSOR THREAD                          ALERT THREAD
 (Producer Thread)                     (Consumer Thread)
          |                                     ^
          v                                     |
    DeviceSensor                                | std::condition_variable
          |                                     | wait(predicate)
          v                                     |
    /dev/medisave (Ring 0)                      |
          |                                     |
          +==== std::queue<StorageAlert> =======+
                (Protected by std::mutex)
```

---

## 2. Thread Roles & Responsibilities

| Thread | Execution Function | Responsibilities |
| :--- | :--- | :--- |
| **Main CLI Thread** | `main()` | Handles interactive user commands, inventory operations, and dispatches TCP client updates. |
| **Sensor Monitoring Thread** | `ThreadedMonitor::sensorWorker()` | Periodically polls `/dev/medisave` via `DeviceSensor`/`StorageMonitor`, updates shared `MonitoringState` under mutex, and produces alerts when thresholds are breached. |
| **Alert Processing Thread** | `ThreadedMonitor::alertWorker()` | Blocks on `alertCv.wait()` with a predicate, consumes `StorageAlert` events without busy waiting, logs triage notifications, and increments alert counters. |
| **TCP Client Worker Threads** | `TcpServer::handleClient()` | Dedicated threads spawned per accepted TCP client connection to receive facility updates concurrently. |

---

## 3. Synchronization Primitives

### A. Shared State Protection (`std::mutex`)
The environmental storage snapshot is held in:
```cpp
struct MonitoringState {
    double temperature{6.50};
    std::string status{"NORMAL"};
    bool running{false};
    int alertCount{0};
};
```
- Access is guarded using `mutable std::mutex stateMutex` via RAII lock guards:
  ```cpp
  MonitoringState ThreadedMonitor::getState() const {
      std::lock_guard<std::mutex> lock(stateMutex);
      return state;
  }
  ```
- No shared mutable state is accessed without lock acquisition, eliminating data races.

### B. Producer-Consumer Alert Queue (`std::condition_variable`)
Instead of CPU-intensive busy polling, the alert processing thread uses condition variable synchronization:

```cpp
// 1. Producer (Sensor Thread upon detecting CRITICAL/WARNING):
{
    std::lock_guard<std::mutex> lock(queueMutex);
    alertQueue.push(alert);
}
alertCv.notify_one();

// 2. Consumer (Alert Processing Thread):
{
    std::unique_lock<std::mutex> lock(queueMutex);
    alertCv.wait(lock, [this] {
        return !alertQueue.empty() || !running;
    });

    if (!alertQueue.empty()) {
        alert = alertQueue.front();
        alertQueue.pop();
    }
}
```
- **Spurious Wakeup Prevention:** The predicate lambda `[this] { return !alertQueue.empty() || !running; }` ensures the thread re-checks condition state upon waking.
- **Lock Granularity:** The mutex is released before formatting and printing alerts to standard output, minimizing contention between producer and consumer.

---

## 4. Graceful Thread Teardown & Lifecycle

The lifecycle strictly follows deterministic RAII rules:
1. `stop()` sets `running = false`.
2. `alertCv.notify_all()` unblocks any sleeping consumer threads.
3. The sensor thread breaks out of its interruptible sleep loop (sliced into 100ms intervals).
4. `sensorThread.join()` and `alertThread.join()` wait for thread execution to terminate cleanly.
5. Mutexes and condition variables are never destroyed while worker threads are executing.

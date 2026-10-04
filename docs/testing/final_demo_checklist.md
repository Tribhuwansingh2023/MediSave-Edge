# MediSave Edge — Final Trainer Demonstration Checklist

This checklist verifies all technical milestones and operational guarantees for the MediSave Edge end-to-end demonstration.

## 1. Build & Environment Preparation
- [x] Project builds completely via `make` without errors or warnings.
- [x] All 8 test suites pass via `make test`.
- [x] Linux kernel character driver builds via `make driver` (or `make -C driver`).
- [x] Driver module loads cleanly: `sudo insmod driver/medisave_driver.ko`.
- [x] Character device node exists and permissions set: `ls -l /dev/medisave && sudo chmod 666 /dev/medisave` (temporary prototype testing; production uses udev rule mode 0660).

> **EVALUATION & LOCAL RUN GUIDE:**  
> For the complete step-by-step demonstration walkthrough and teacher viva defense Q&A, refer to [**`../../GUIDE.md`**](../../GUIDE.md).

---

## 2. Core Functional Modules
- [x] **Inventory Management**:
  - Add medicine record with valid ID, batch, bounds, and expiry.
  - Search by ID and name substring.
  - Update stock quantity and verify low-stock detection.
  - Remove medicine and verify persistence to `data/medicines.txt`.
- [x] **Expiry Management**:
  - Detect expired medicines with negative day offsets.
  - Detect expiring soon medicines within 30 days.
  - Generate priority-sorted alert queue (Max-Heap) placing critical items at top.
- [x] **Linux Character Device Driver & HAL**:
  - Read storage temperature via `TemperatureMonitor` and `DeviceSensor`.
  - Simulate temperature change to 6.50 °C (NORMAL).
  - Simulate temperature excursion to 9.20 °C (WARNING).
  - Simulate temperature excursion to 11.50 °C (CRITICAL).
  - Verify graceful fallback when driver is not loaded.
- [x] **Process Lifecycle & IPC**:
  - Spawn worker process via `fork()` and `exec()`.
  - Transmit telemetry through anonymous pipe.
  - Synchronize shared memory block using POSIX semaphore.
  - Handle asynchronous signal `SIGUSR1` for live status polling.
  - Graceful termination of child process via `SIGTERM` and `waitpid()`.
- [x] **Concurrency & Multithreading**:
  - Launch `ThreadedMonitor` spawning Sensor and Alert worker threads.
  - Verify thread-safe synchronization using `std::mutex`.
  - Verify alert triage queue signaling via `std::condition_variable`.
  - Stop threads cleanly on request without deadlocks.
- [x] **TCP Networking & Multi-Facility Communication**:
  - Launch multi-threaded TCP server (`TcpServer`) on port 5000.
  - Dispatch facility inventory updates using TCP client (`TcpClient`).
  - Transmit pipe-delimited payload: `Facility-A|Paracetamol|P2026A|150|SURPLUS`.
  - Transmit pipe-delimited payload: `Facility-B|Paracetamol|P2026A|20|SHORTAGE`.
  - Receive automated server acknowledgement (`ACK|Facility-A`).
- [x] **Redistribution Decision Engine**:
  - Classify surplus and shortage inventory across facilities.
  - Generate deterministic matching recommendation: `Facility-A -> Facility-B (30 units)`.
  - Prioritize larger shortages first.
  - Verify transfer constraints: never negative, zero, or exceeding source surplus.
  - Display advisory notice: recommendations are decision-support outputs.
- [x] **Host System Monitoring (`/proc`)**:
  - Read processor model and core topology from `/proc/cpuinfo`.
  - Sample multi-counter CPU utilization from `/proc/stat`.
  - Extract physical memory totals and compute utilization from `/proc/meminfo`.
  - Read system uptime from `/proc/uptime`.
  - Display formatted System Health report.
- [x] **Executive Dashboard**:
  - Select Menu Option 17 (Show System Dashboard).
  - Verify real-time presentation of Storage, Inventory, Redistribution, Host System, and Services.
- [x] **Shutdown Sequence**:
  - Select Menu Option 18 (Exit) or send `SIGINT` (Ctrl+C).
  - Verify orderly shutdown: thread termination, process cleanup, network server shutdown, file persistence save, and sensor disconnection.

---

## 3. Documentation & Repository Integrity
- [x] `README.md` completely updated with implemented features and constraints.
- [x] Architecture documents created for Redistribution and System Monitoring.
- [x] Stage 6 feature summary documented in `docs/progress/stage6_final_features.md`.
- [x] Git repository status verified with clean working tree.

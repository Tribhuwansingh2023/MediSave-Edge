# MediSave Edge — Stage 5 Progress Report
## Stage 5: Multithreading & TCP Client/Server Facility Communication

**Student / Author:** Tribhuwan Singh  
**Project:** MediSave Edge — Linux-Based Medicine Storage Monitoring, Inventory Alert & Redistribution Decision System  
**Component:** Stage 5 — C++ Multithreading & TCP Client/Server Socket Network  
**Date:** October 2026  

---

## 1. Objective

Stage 5 expands the MediSave Edge system by introducing concurrent in-process worker threads and an inter-facility networking layer:
1. **Multithreading:** In-process concurrency using `std::thread`, `std::mutex`, `std::condition_variable`, and a producer-consumer alert queue.
2. **TCP Client/Server:** Point-to-point inter-facility communication simulating distributed medicine storage depots exchanging surplus and shortage notices.
3. **Redistribution Preparation:** In-memory structured buffering of remote facility requests ready for consumption by Stage 6.

---

## 2. Multithreading Architecture & Synchronization

- **Sensor Thread:** Periodically samples `/dev/medisave` via `DeviceSensor`/`StorageMonitor` and updates `MonitoringState` under `std::mutex`.
- **Alert Thread:** Blocks on `std::condition_variable` and wakes instantaneously when an excursion alert (`StorageAlert`) is pushed to the queue.
- **Mutex Protection:** `std::lock_guard` and `std::unique_lock` ensure race-free state queries and queue operations.
- **Graceful Teardown:** Atomic boolean `running` combined with `condition_variable::notify_all()` ensures all worker threads unblock and join cleanly without thread detachment.

---

## 3. TCP Architecture & Message Protocol

- **Server (`TcpServer` / `bin/medisave_server`):** Listens on `127.0.0.1:5000` (configurable), accepts incoming facility connections, dispatches a worker thread per client, logs incoming reports, and issues acknowledgements.
- **Client (`TcpClient` / `bin/medisave_client`):** Connects to server, dispatches facility inventory state, and processes acknowledgements.
- **Text Protocol:**
  - `FACILITY|MEDICINE|BATCH|QUANTITY|TYPE\n`
  - Responses: `ACK|FACILITY\n` or `ERR|INVALID_FORMAT\n`

---

## 4. Multi-Facility Simulation

Facilities broadcast their surplus and deficit positions to the coordination server:
```text
Facility-A|Paracetamol|P2026A|150|SURPLUS
Facility-B|Paracetamol|P2026A|20|SHORTAGE
```
The server buffers these entries in a thread-safe structure (`struct FacilityMessage`), establishing the foundation for Stage 6 redistribution optimization.

---

## 5. Automated Verification & Test Results

1. **Multithreading Unit Tests (`bin/thread_test`):**
   - Sensor thread startup: PASS
   - Mutex state synchronization: PASS
   - Condition variable signaling: PASS
   - Alert queue delivery: PASS
   - Graceful thread join: PASS
2. **TCP Socket Unit Tests (`bin/tcp_test`):**
   - Server startup & bind: PASS
   - Client connection: PASS
   - Message transmission: PASS
   - Server acknowledgement response: PASS
   - Client disconnect: PASS
   - Server shutdown: PASS
3. **Master Suite (`make test`):**
   - 35/35 Inventory unit tests PASS
   - 14/14 Device sensor integration tests PASS
   - 6/6 IPC tests PASS
   - Process lifecycle test PASS
   - Thread test PASS
   - TCP test PASS

---

## 6. Known Limitations
- Graph matching and priority-weighted redistribution algorithms are deferred to Stage 6.
- External internet routable networking is intentionally avoided; communication is modeled on localhost.

---

## 7. Next Milestone (Stage 6)
- **Redistribution Decision Engine:** Automated matching algorithm pairing shortage facilities with nearest surplus facilities based on expiry urgency and stock limits.
- **System Monitoring Dashboard:** Integrated telemetry and network overview.
- **Final Packaging & End-to-End Evaluation.**

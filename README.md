# MediSave Edge
### Linux-Based Medicine Storage Monitoring, Inventory Alert and Redistribution Decision System

**Student Name:** Tribhuwan Singh  
**Project Type:** Individual Capstone Project  
**Domain:** Software & Hardware Architecture / Linux System Programming  
**Implementation Languages:** C (Linux Kernel Module) / C++17 (User-space Engine)  
**Target Environment:** Linux  

---

## 1. Project Overview

**MediSave Edge** is an integrated, high-reliability Linux system designed for real-time monitoring of pharmaceutical storage conditions, intelligent inventory balance analysis, automated triage of impending expirations, and decision-support redistribution across distributed healthcare facilities.

The project demonstrates complete vertical system integration:
* **Ring 0 (Kernel Space):** Real Linux Character Device Driver (`/dev/medisave`) with fixed-point arithmetic, mutual exclusion, and an IOCTL control plane.
* **Ring 3 (User Space):** Modern C++17 architecture employing multi-process isolation (`fork`, `exec`, `waitpid`), streaming and zero-copy IPC (`pipe`, POSIX shared memory, POSIX named semaphores), signal handlers (`sigaction`), multi-threading (`std::thread`, `std::mutex`, `std::condition_variable`), and concurrent TCP networking.
* **Decision Support:** Automated surplus and shortage classification, deterministic priority-driven redistribution recommendations, and direct virtual filesystem host monitoring (`/proc`).

> **CLINICAL & REGULATORY NOTICE:**  
> Redistribution recommendations are advisory software outputs and do not automatically execute physical medicine transfers. All redistribution suggestions are decision-support outputs requiring human verification and clinical oversight.

---

## 2. Features

### Core C++
* **Object-Oriented Design & Modern C++17:** Clean separation of concerns with encapsulated domain classes (`Medicine`, `InventoryManager`, `DeviceSensor`, `TemperatureMonitor`, `RedistributionEngine`, `SystemMonitor`).
* **STL Containers & Algorithms:** High-performance data structures including `std::unordered_map` for $O(1)$ lookups, `std::vector`, `std::queue`, `std::priority_queue` (Max-Heap), and deterministic sorting algorithms.
* **Inventory Management:** Full CRUD operations (Add, Remove, Update, Search by ID and Name substring, and Quantity adjustments).
* **Robust File Persistence:** Flat-file serialization and deserialization at `data/medicines.txt`.
* **Expiry Analysis & Triage:** Calendar delta calculations tracking days until expiry, identifying expired batches, and ranking items expiring within 30 days.

### Linux Systems Programming
* **Linux Character Device Driver:** Loadable kernel module (`driver/medisave_driver.c`) registering `/dev/medisave` with VFS file operations (`open`, `read`, `write`, `unlocked_ioctl`, `release`).
* **Kernel/User-Space Communication:** Memory-safe exchanges via `copy_to_user()` / `copy_from_user()` and binary IOCTL control plane (`medisave_ioctl.h`).
* **Multi-Process Management:** Child process spawning via `fork()`, image replacement via `execl()`, and clean termination / harvest via `waitpid()`.
* **Inter-Process Communication (IPC):**
  * Anonymous streaming pipes (`pipe()`) with strict descriptor management.
  * POSIX Shared Memory (`shm_open()`, `mmap()`) for zero-copy telemetry.
  * POSIX Named Semaphore (`sem_open()`, `sem_wait()`, `sem_post()`) preventing torn reads.
* **POSIX Signals:** Async-signal-safe handlers (`sigaction()`) for `SIGINT`, `SIGTERM`, and `SIGUSR1`.
* **Host System Monitoring (`/proc`):** Direct virtual filesystem parsing of `/proc/cpuinfo`, `/proc/stat`, `/proc/meminfo`, and `/proc/uptime` without external dependencies.

### Concurrency & Multithreading
* **In-Process Concurrency:** Multi-threaded storage monitoring utilizing `std::thread`.
* **Mutual Exclusion:** Thread-safe state access protected by `std::mutex` and RAII `std::lock_guard`.
* **Condition Variables:** Producer-consumer alert triage queue utilizing `std::condition_variable` to eliminate CPU busy-waiting.

### Networking & Distributed Telemetry
* **TCP Server:** Multi-threaded socket server (`TcpServer`) spawning client handler threads.
* **TCP Client:** Non-blocking connection dispatcher (`TcpClient`) transmitting structured facility telemetry.
* **Multi-Facility Protocol:** Pipe-delimited wire messages (`FACILITY|MEDICINE|BATCH|QUANTITY|TYPE\n`) and automated server acknowledgements (`ACK|FACILITY\n`).

### Decision Support & Monitoring
* **Stock Classification:** Evaluates stock against min/max thresholds (`SHORTAGE`, `NORMAL`, `SURPLUS`).
* **Redistribution Engine:** Matches surplus source facilities with shortage destination facilities based on medicine identity, compatibility, and availability.
* **Deterministic Priority Order:** Prioritizes largest shortages first, earlier expiry dates, and deterministic name/ID ties.
* **Storage Condition Alerts:** Real-time classification (`LOW`, `NORMAL`, `WARNING`, `CRITICAL`) with priority triage queue integration.
* **Executive Dashboard:** Compact main dashboard presenting live storage, inventory, redistribution, host CPU/RAM, and background service statuses.

---

## 3. Project Structure

```text
MediSave-Edge/
├── Makefile                        # Master build system
├── README.md                       # Comprehensive documentation
├── .gitignore                      # Git ignore rules
├── LICENSE                         # MIT License
│
├── include/                        # C / C++ Header files
│   ├── medicine.h                  # Medicine entity declaration
│   ├── inventory_manager.h         # Inventory manager declaration
│   ├── expiry_utils.h              # Date calculation & expiry utilities
│   ├── alert_system.h              # Priority queue alert system
│   ├── DeviceSensor.h              # Low-level POSIX driver wrapper
│   ├── StorageMonitor.h            # High-level storage monitor
│   ├── TemperatureMonitor.h        # Centralized storage monitoring & history
│   ├── RedistributionEngine.h      # Decision-support redistribution engine
│   ├── SystemMonitor.h             # Linux /proc host telemetry & dashboard
│   ├── medisave_ioctl.h            # Shared kernel/user IOCTL definitions
│   ├── SharedData.h                # POSIX shared memory layout
│   ├── IPCManager.h                # Pipe, shared memory, and semaphore manager
│   ├── ProcessManager.h            # Process lifecycle (fork/exec/waitpid) manager
│   ├── ThreadedMonitor.h           # In-process C++17 multithreading engine
│   ├── SocketCompat.h              # Cross-platform socket abstraction
│   ├── TcpProtocol.h               # Wire text protocol & FacilityMessage
│   ├── TcpServer.h                 # Multi-client TCP server
│   ├── TcpClient.h                 # TCP client implementation
│   └── NetworkManager.h            # High-level network coordinator
│
├── src/                            # C++ Implementation files
│   ├── medicine.cpp                # Medicine logic & serialization
│   ├── inventory_manager.cpp       # Inventory CRUD & persistence
│   ├── expiry_utils.cpp            # Calendar difference algorithms
│   ├── alert_system.cpp            # Heap ordering & alert rendering
│   ├── DeviceSensor.cpp            # POSIX driver system call wrapper
│   ├── StorageMonitor.cpp          # Storage condition analysis
│   ├── TemperatureMonitor.cpp      # Central temperature telemetry & alerts
│   ├── RedistributionEngine.cpp    # Deterministic surplus/shortage matching
│   ├── SystemMonitor.cpp           # Direct /proc VFS metrics & dashboard
│   ├── IPCManager.cpp              # Anonymous pipe, shm, and sem implementations
│   ├── ProcessManager.cpp          # fork, exec, waitpid, and kill implementation
│   ├── ThreadedMonitor.cpp         # std::thread, mutex, and condition_variable implementation
│   ├── TcpServer.cpp               # Socket bind/listen/accept & client worker threads
│   ├── TcpClient.cpp               # Socket connect/send/recv implementation
│   ├── NetworkManager.cpp          # High-level network lifecycle coordinator
│   ├── monitor_worker.cpp          # Standalone background monitoring executable
│   ├── medisave_server.cpp         # Standalone TCP facility coordination hub
│   ├── medisave_client.cpp         # Standalone TCP facility update client
│   └── main.cpp                    # Final 18-option CLI application
│
├── driver/                         # Linux Kernel Module
│   ├── medisave_driver.c           # Character device driver source
│   ├── Makefile                    # Kernel module build script
│   └── README.md                   # Driver documentation & guide
│
├── tests/                          # Automated Test Suites (8 Suites)
│   ├── test_inventory.cpp          # 35 automated inventory tests
│   ├── device_sensor_test.cpp      # 14 automated driver integration tests
│   ├── driver_test.cpp             # Direct driver verification
│   ├── ipc_test.cpp                # Pipe, shm, and semaphore unit tests
│   ├── process_test.cpp            # fork, exec, and waitpid lifecycle tests
│   ├── thread_test.cpp             # Multithreading, mutex & CV unit tests
│   ├── tcp_test.cpp                # TCP server, client & protocol unit tests
│   ├── redistribution_test.cpp     # Redistribution matching & bounds tests
│   └── system_monitor_test.cpp     # Linux /proc filesystem parsing tests
│
├── data/                           # Data directory
│   └── medicines.txt               # Persistent inventory storage
│
└── docs/                           # Documentation
    ├── architecture/
    │   ├── device_driver_architecture.md   # Kernel driver architecture
    │   ├── cpp_driver_integration.md       # Driver integration architecture
    │   ├── process_ipc_architecture.md     # Multi-process & IPC architecture
    │   ├── multithreading_architecture.md  # Concurrency & CV architecture
    │   ├── tcp_architecture.md             # TCP networking & socket lifecycle
    │   ├── redistribution_architecture.md  # Redistribution engine architecture
    │   └── system_monitoring_architecture.md# Linux /proc telemetry architecture
    ├── progress/
    │   ├── day1_inventory.md               # Day 1 Inventory milestone report
    │   ├── day1_device_driver.md           # Day 1 Driver demo script
    │   ├── task4_driver_integration.md     # Task 4 integration report
    │   ├── task5_process_ipc.md            # Task 5 process & IPC report
    │   ├── task6_multithreading_tcp.md     # Task 6 multithreading & TCP report
    │   └── task7_final_features.md         # Task 7 final feature report
    └── testing/
        └── final_demo_checklist.md         # Trainer demo verification checklist
```

---

## 4. Build Instructions

### Prerequisites (Ubuntu/Debian):
```bash
sudo apt update
sudo apt install -y build-essential linux-headers-$(uname -r) make gcc g++
```

### Compiling All Targets:
```bash
make all
# Compiles main app (bin/medisave), worker, server, client, and all 8 test binaries
```

### Compiling Linux Character Device Driver:
```bash
make driver
# Compiles driver/medisave_driver.ko
```

---

## 5. Automated Unit & Integration Testing

Run all 8 automated test suites with a single command:
```bash
make test
```

Verification includes:
1. **Inventory Unit Tests (`bin/test_inventory`)**: 35 tests verifying CRUD, search, expiry calculation, priority queues, and file persistence.
2. **Device Sensor Integration Tests (`bin/device_sensor_test`)**: 14 tests verifying IOCTL operations, VFS read/write, and parameter boundary validation.
3. **IPC Tests (`bin/ipc_test`)**: Tests anonymous pipes, POSIX shared memory, and semaphore synchronization.
4. **Process Lifecycle Tests (`bin/process_test`)**: Tests `fork()`, `exec()`, and `waitpid()` harvesting.
5. **Multithreading Tests (`bin/thread_test`)**: Tests `std::thread`, mutex synchronization, and condition variable triage.
6. **TCP Socket Tests (`bin/tcp_test`)**: Tests server startup, client connection, message exchange, and clean shutdown.
7. **Redistribution Engine Tests (`bin/redistribution_test`)**: Tests surplus/shortage detection, matching, transfer limits, and deterministic ordering.
8. **Linux /proc System Monitor Tests (`bin/system_monitor_test`)**: Tests `/proc/cpuinfo`, `/proc/stat`, `/proc/meminfo`, and `/proc/uptime` direct parsing.

---

## 6. End-to-End Trainer Demonstration Workflow

To execute the complete 17-step end-to-end demonstration:

```bash
# STEP 1: Build and load Linux character device driver
cd driver && make && sudo insmod medisave_driver.ko && sudo chmod 666 /dev/medisave && cd ..

# STEP 2: Launch main MediSave Edge CLI
./bin/medisave

# STEP 3: Display Inventory (Option 5)
# Displays all active medicine records

# STEP 4: Show Expiry Alerts (Option 8)
# Displays priority max-heap alert queue with expired and critical items

# STEP 5: Read Storage Temperature (Option 9)
# Reads current temperature (default 6.50 °C - NORMAL)

# STEP 6: Set Simulated Temperature to 11.50 °C (Option 10)
# Writes 11.50 °C into /dev/medisave via IOCTL

# STEP 7: Show CRITICAL Storage Alert (Option 8 / 11)
# Storage chamber evaluates status as CRITICAL and issues urgent warning

# STEP 8: Start Background Monitoring (Option 12)
# Spawns sensor and alert worker threads and starts in-process TCP server

# STEP 9: Transmit Facility Updates (Option 14)
# Send Facility-A surplus: Facility-A | Paracetamol | P2026A | 150 | SURPLUS
# Send Facility-B shortage: Facility-B | Paracetamol | P2026A | 20 | SHORTAGE

# STEP 10: Run Redistribution Analysis (Option 15)
# Generates deterministic advisory proposal:
# Suggested Transfer: 30 units (Facility-A -> Facility-B)

# STEP 11: Show System Health (Option 16)
# Displays live CPU model, logical cores, memory utilization, and uptime

# STEP 12: Show Executive Dashboard (Option 17)
# Single-screen summary of Storage, Inventory, Redistribution, and Services

# STEP 13: Stop Background Monitoring (Option 13)
# Terminates monitoring threads cleanly

# STEP 14: Exit (Option 18)
# Gracefully saves inventory and shuts down all resources

# STEP 15: Unload driver
sudo rmmod medisave_driver
```

---

## 7. Interactive CLI Menu

```text
========================================
           MEDISAVE EDGE
========================================
 1. Add Medicine
 2. Remove Medicine
 3. Update Medicine
 4. Search Medicine
 5. Display Inventory
 6. Update Stock
 7. Check Expiry
 8. Show Alerts
 9. Read Storage Temperature
10. Set Simulated Temperature
11. Show Storage Condition
12. Start Monitoring
13. Stop Monitoring
14. Send Facility Update
15. Analyze Redistribution
16. Show System Health
17. Show System Dashboard
18. Exit
========================================
```

---

## 8. License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

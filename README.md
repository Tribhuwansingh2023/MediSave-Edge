# MediSave Edge
### Linux-Based Medicine Storage Monitoring, Inventory Alert and Redistribution Decision System

| Attribute | Details |
| :--- | :--- |
| **Student Name** | Tribhuwan Singh |
| **Project Type** | Individual Capstone Project |
| **Domain** | Software & Hardware Architecture / Linux System Programming |
| **Implementation Languages** | C (Linux Kernel Module, C99) / C++17 (User-space Engine) |
| **Target Environment** | Linux (Ubuntu 20.04+ / Debian 11+, x86_64) |

---

## Table of Contents

- [1. Project Overview](#1-project-overview)
- [2. Problem Statement](#2-problem-statement)
- [3. Objectives](#3-objectives)
- [4. Key Features](#4-key-features)
- [5. Technology Stack](#5-technology-stack)
- [6. System Architecture](#6-system-architecture)
- [7. User Space and Kernel Space](#7-user-space-and-kernel-space)
- [8. Device Driver Architecture](#8-device-driver-architecture)
- [9. Process Architecture](#9-process-architecture)
- [10. IPC Architecture](#10-ipc-architecture)
- [11. Multithreading Architecture](#11-multithreading-architecture)
- [12. TCP Architecture](#12-tcp-architecture)
- [13. Redistribution Architecture](#13-redistribution-architecture)
- [14. Linux System Monitoring (`/proc`)](#14-linux-system-monitoring-proc)
- [15. Project Structure](#15-project-structure)
- [16. Requirements](#16-requirements)
- [17. Installation](#17-installation)
- [18. Running the Project](#18-running-the-project)
- [19. Testing](#19-testing)
- [20. Limitations](#20-limitations)
- [21. Future Scope](#21-future-scope)
- [22. Author](#22-author)
- [23. AI Assistance](#23-ai-assistance)

---

## 1. Project Overview

**MediSave Edge** is a Linux-based medicine storage monitoring, inventory alert, and redistribution decision-support prototype implemented using modern C++17 and low-level Linux systems programming concepts.

The system is designed as an operational prototype and decision-support monitoring tool. It continuously tracks pharmaceutical environmental conditions, identifies impending shelf-life expirations, ranks inventory risks using prioritized alert queues, and calculates deterministic redistribution proposals across distributed healthcare facilities.

> **CLINICAL & REGULATORY NOTICE:**  
> MediSave Edge is an advisory decision-support software prototype. It does **not** make autonomous medical diagnoses, establish treatment regimens, or automatically execute physical drug transfers. All redistribution proposals require human review and authorization by certified medical officers and pharmacists.

> **EVALUATION & LOCAL RUN GUIDE:**  
> Step-by-step run, evaluation, and demo guide: [**`GUIDE.md`**](GUIDE.md).

---

## 2. Problem Statement

Public health facilities and decentralized healthcare depots routinely face critical inventory imbalances:
* **Thermal Spoilage Risks**: Temperature-sensitive pharmaceuticals require strict storage maintenance. Undetected cooling failures cause silent efficacy loss.
* **Unmonitored Shelf-Life Expiry**: Medicines expire unconsumed due to lack of automated tracking, resulting in financial loss and wasted supplies.
* **Regional Supply Imbalances**: Acute drug shortages in rural clinics often occur simultaneously with surplus overstock in central district warehouses.
* **Manual Tracking Overhead**: Manual clipboard checks and fragmented spreadsheets fail to provide real-time alerts or timely re-allocation decisions.

MediSave Edge solves these challenges through automated environmental sensing, proactive Max-Heap triage, concurrent multi-facility networking, and deterministic redistribution calculations.

---

## 3. Objectives

The project accomplishes the following technical and operational objectives:
1. **Core Inventory Management**: Deliver an in-memory CRUD engine backed by persistent disk storage.
2. **Automated Expiry Triage**: Calculate calendar offsets to quarantine expired batches and flag items expiring within 7 and 30 days.
3. **Storage Temperature Monitoring**: Maintain continuous chamber telemetry with configurable safety thresholds.
4. **Linux Character Device Driver Integration**: Interface user-space applications directly with a custom loadable kernel module (`/dev/medisave`) via VFS and IOCTL.
5. **Process Lifecycle & IPC**: Implement multi-process monitoring using `fork()`, `exec()`, `waitpid()`, anonymous pipes, POSIX shared memory, and POSIX named semaphores.
6. **Concurrent In-Process Multithreading**: Decouple sensor acquisition and alert processing using `std::thread`, `std::mutex`, and `std::condition_variable`.
7. **Distributed TCP Communication**: Operate multi-client TCP server and client socket pipelines to exchange facility inventory telemetry over port 5000.
8. **Redistribution Decision Support**: Deterministically match regional surpluses with shortages under strict transfer constraints.
9. **Linux Host Telemetry**: Parse the Linux `/proc` virtual filesystem (`/proc/cpuinfo`, `/proc/stat`, `/proc/meminfo`, `/proc/uptime`) with zero third-party dependencies.
10. **Integrated Executive Dashboard**: Consolidate live chamber telemetry, inventory levels, redistribution proposals, and host health into a single-screen view.

---

## 4. Key Features

### Inventory
* **CRUD Engine**: Add, remove, update, and search medicines by ID and name substring.
* **Stock Monitoring**: Dynamic threshold validation detecting depleted stock (qty <= minStock).
* **File Persistence**: Serialization and deserialization from `data/medicines.txt`.

### Expiry
* **Expired Drug Detection**: Negative day calculation isolating expired batches for immediate quarantine.
* **Expiring-Soon Detection**: Tiered warnings for batches expiring within 7 days (critical) and 30 days (warning).
* **Priority Alerts**: STL Max-Heap (`std::priority_queue`) prioritizing urgent risks at the top.

### Linux Device Driver
* **Character Device**: Dynamically registered kernel module exposing `/dev/medisave`.
* **VFS Operations**: Standard `open()`, `read()`, `write()`, and `release()` system calls.
* **IOCTL Control Plane**: Fast binary data exchange via `MEDISAVE_IOC_SET_TEMP`, `MEDISAVE_IOC_GET_TEMP`, `MEDISAVE_IOC_GET_STATUS`, and `MEDISAVE_IOC_GET_DATA`.
* **Hardware Simulation**: Integer fixed-point milli-Celsius representation prohibiting kernel floating-point operations.

### Linux System Programming
* **Processes**: Process duplication via `fork()`, executable replacement via `execl()`, and non-blocking zombie prevention via `waitpid()`.
* **Anonymous Pipes**: Unidirectional streaming IPC transferring live telemetry from worker to parent.
* **POSIX Shared Memory**: Zero-copy segment (`/medisave_shm_v1`) mapped via `shm_open()` and `mmap()`.
* **POSIX Named Semaphore**: Concurrency lock (`/medisave_sem_v1`) preventing torn reads during shared memory access.
* **POSIX Signals**: Async-signal-safe handlers (`sigaction()`) intercepting `SIGINT`, `SIGTERM`, and `SIGUSR1`.

### Multithreading
* **Sensor Polling Thread**: Background thread querying `/dev/medisave` at periodic intervals.
* **Alert Consumer Thread**: Worker thread waiting on a condition variable to triage excursions without CPU busy-waiting.
* **Synchronization**: RAII `std::lock_guard<std::mutex>` protecting shared telemetry.
* **Thread Teardown**: Clean atomic coordination and `join()` execution.

### Networking
* **TCP Server**: Multi-client socket server spawning dedicated threads per facility connection.
* **TCP Client**: Socket connection dispatcher transmitting facility status payloads.
* **Protocol & ACK**: Pipe-delimited wire format (`FACILITY|MEDICINE|BATCH|QUANTITY|TYPE\n`) and automated `ACK|FACILITY\n`.

### Redistribution
* **Surplus & Shortage Analysis**: Classification of facility stocks relative to minimum requirements.
* **Deterministic Matching**: Prioritization of highest deficits, earliest expiries, and deterministic name/ID ties.
* **Transfer Constraints**: Transfers strictly bounded by min(Surplus, Shortage); never negative or zero.
* **Advisory Disclaimer**: Software proposals that do not execute automatic physical movements.

### System Monitoring
* **/proc/cpuinfo**: Processor model identification and logical core topology.
* **/proc/stat**: Multi-sample delta calculation for accurate CPU load percentage.
* **/proc/meminfo**: Physical memory metrics (`MemTotal`, `MemAvailable`, memory used percentage).
* **/proc/uptime**: Conversion of kernel uptime seconds into human-readable duration strings.

---

## 5. Technology Stack

* **Programming Languages**: C (Linux Kernel Module, C99) / C++ (User-space Engine, C++17).
* **Operating System**: Linux (Ubuntu 20.04+ / Debian 11+ or compatible Linux distribution, x86_64).
* **Build System**: GNU Make, GCC / G++ toolchain with C++17 support.
* **Linux Kernel Subsystems**: Loadable Kernel Module (LKM), Character Device Subsystem, VFS, IOCTL, Kernel Mutexes.
* **System Programming APIs**: POSIX System Calls (`fork`, `exec`, `waitpid`, `pipe`, `shm_open`, `mmap`, `sem_open`, `sigaction`).
* **Networking**: POSIX Berkeley Sockets (TCP/IP, IPv4, stream sockets).
* **Concurrency**: `std::thread`, `std::mutex`, `std::condition_variable`, `std::atomic`.
* **STL Data Structures**: `std::unordered_map` (inventory lookups), `std::priority_queue` (alert triage), `std::vector`, `std::queue`.

---

## 6. System Architecture

```text
                         MEDISAVE EDGE
                               |
        -------------------------------------------------
        |                    |                         |
        v                    v                         v
    INVENTORY           STORAGE MONITORING       FACILITY NETWORK
        |                    |                         |
        |               DeviceSensor                  |
        |                    |                        TCP
        |               /dev/medisave                  |
        |                    |                         |
        ---------------------|--------------------------
                             v
                       ALERT ENGINE
                             |
               ---------------------------
               |                         |
               v                         v
         EXPIRY ALERTS             STOCK ANALYSIS
                                         |
                                         v
                               REDISTRIBUTION ENGINE
                                         |
                                         v
                                 SYSTEM DASHBOARD
                                         |
                          ----------------------------
                          |            |             |
                         CPU         MEMORY        UPTIME
                        /proc        /proc         /proc
```

---

## 7. User Space and Kernel Space

```text
================================================================================
                                USER SPACE (Ring 3)
================================================================================
  C++ Application (bin/medisave)
  - DeviceSensor (Hardware Abstraction Layer)
  - TemperatureMonitor & StorageMonitor
  - InventoryManager & AlertSystem
  - RedistributionEngine & NetworkManager
  - SystemMonitor

          |
          | open(), read(), write(), ioctl() system calls
          v
================================================================================
                                KERNEL SPACE (Ring 0)
================================================================================
  Linux VFS (Virtual File System)
       |
       v
  Character Device Driver (/dev/medisave)
  - driver/medisave_driver.c
  - copy_to_user() & copy_from_user()
  - DEFINE_MUTEX(medisave_mutex)
  - Simulated Hardware State: current_temp_milli (e.g. 6500 = 6.50 °C)
================================================================================
```

---

## 8. Device Driver Architecture

```text
C++ Application (DeviceSensor)
      |
      v
open() / read() / write() / ioctl()
      |
      v
/dev/medisave (dynamic major number (alloc_chrdev_region))
      |
      v
Character Device Driver (medisave_driver.c)
      |
      +---> copy_to_user() / copy_from_user()
      +---> Fixed-point integer temperature state (milli-Celsius)
      +---> Mutex concurrency protection (medisave_mutex)
      +---> IOCTL Control Plane (medisave_ioctl.h)
      |
      v
Simulated Environmental Sensor Telemetry
```

---

## 9. Process Architecture

```text
Main Application Process (bin/medisave)
     |
     +---- fork()
     |
     +---- Child Process
                |
                +---- exec() -> execl("bin/monitor_worker", ...)
                      |
                      v
                Dedicated Monitor Worker (bin/monitor_worker)
                      |
                      +---> Continuous sampling & IPC transmission
                      |
                      v
     Main Process calls waitpid(pid, &status, WNOHANG) -> Zero Zombie Processes
```

---

## 10. IPC Architecture

### Pipe IPC
```text
Monitor Worker Process
   |
   | write(pipefd[1], buffer, len)
   v
 Anonymous Pipe Buffer (Kernel Memory)
   |
   | read(pipefd[0], buffer, len)
   v
Main Application Process
```

### Shared Memory & Semaphore Synchronization
```text
Producer Process (monitor_worker)             Consumer Process (main)
         |                                              |
         | sem_wait(/medisave_sem_v1)                   | sem_wait(/medisave_sem_v1)
         v                                              v
+---------------------------------------------------------------+
|         POSIX Shared Memory Segment (/medisave_shm_v1)        |
|                  struct SharedMonitorData                     |
+---------------------------------------------------------------+
         |                                              |
         | sem_post(/medisave_sem_v1)                   | sem_post(/medisave_sem_v1)
         v                                              v
```

---

## 11. Multithreading Architecture

```text
Main Application Process
      |
      +---- Sensor Polling Thread (ThreadedMonitor::sensorWorker)
      |     - Samples /dev/medisave every 5 seconds
      |     - Updates shared state protected by std::mutex
      |     - Signals alertCv upon storage excursion
      |
      +---- Alert Consumer Thread (ThreadedMonitor::alertWorker)
      |     - Waits on std::condition_variable (zero CPU busy-waiting)
      |     - Consumes alerts from std::queue<StorageAlert>
      |
      +---- Network Worker Threads (TcpServer::handleClient)
            - Dedicated thread per accepted facility TCP connection
```

---

## 12. TCP Architecture

```text
Facility Client A (bin/medisave_client)        Facility Client B (bin/medisave_client)
(e.g., Regional Depot)                         (e.g., District Clinic)
       |                                              |
       | connect(127.0.0.1:5000)                      | connect(127.0.0.1:5000)
       v                                              v
+---------------------------------------------------------------+
|               MediSave TCP Server (bin/medisave_server)       |
|                 socket() -> bind() -> listen() -> accept()    |
+---------------------------------------------------------------+
       |                                              |
       v                                              v
 [Worker Thread A]                              [Worker Thread B]
 recv() -> parse message                        recv() -> parse message
 send(ACK|Facility-A)                           send(ACK|Facility-B)
 close()                                        close()
```

---

## 13. Redistribution Architecture

```text
Distributed Facility Telemetry (TCP Buffer & Local Stock)
       |
       v
Stock Classification (SHORTAGE, NORMAL, SURPLUS)
       |
       v
Surplus / Shortage Delta Analysis
- Surplus  = Quantity - MinimumRequired
- Shortage = MinimumRequired - Quantity
       |
       v
Deterministic Matching Engine
- Match same medicine ID / name
- Verify compatible batches
- Prioritize larger shortage deficit first
- Prioritize earlier expiry date first
       |
       v
Transfer Quantity Calculation
- Suggested Transfer = min(Surplus_source, Shortage_dest)
       |
       v
Advisory Redistribution Recommendation
(Facility A ---> Facility B)
```

> Note: All redistribution proposals are strictly advisory decision-support recommendations requiring clinical and administrative sign-off.

---

## 14. Linux System Monitoring (`/proc`)

MediSave Edge directly parses the virtual filesystem without third-party monitoring libraries:
* **/proc/cpuinfo**: Extracts processor model string (`model name` / `Hardware`) and counts logical processor instances.
* **/proc/stat**: Samples cumulative CPU counters across a 50–100ms interval to compute delta utilization percentage:
  `CPU Usage % = ((delta_Total - delta_Idle) / delta_Total) * 100`
* **/proc/meminfo**: Reads `MemTotal` and `MemAvailable` to compute active memory consumption in gigabytes and percentage.
* **/proc/uptime**: Reads elapsed kernel uptime seconds and formats into duration strings (e.g., `"2 days, 1 hours 15 minutes"`).

---

## 15. Project Structure

```text
MediSave-Edge/
├── Makefile                                # Master build system
├── README.md                               # Complete project documentation
├── GUIDE.md                                # Evaluation, local run, and trainer demo guide
├── .gitignore                              # Git ignore rules
├── LICENSE                                 # MIT License
│
├── include/                                # C / C++ Header files
│   ├── alert_system.h                      # Max-Heap priority alert system
│   ├── DeviceSensor.h                      # User-space POSIX driver wrapper
│   ├── expiry_utils.h                      # Date calculation & expiry classification
│   ├── inventory_manager.h                 # Unordered-map inventory manager
│   ├── IPCManager.h                        # Pipe, shared memory, and semaphore manager
│   ├── medicine.h                          # Medicine domain entity declaration
│   ├── medisave_ioctl.h                    # Shared kernel/user IOCTL definitions
│   ├── NetworkManager.h                    # High-level network coordinator
│   ├── ProcessManager.h                    # fork, exec, and waitpid lifecycle manager
│   ├── RedistributionEngine.h              # Decision-support redistribution engine
│   ├── SharedData.h                        # POSIX shared memory data layout
│   ├── SocketCompat.h                      # Cross-platform socket abstraction
│   ├── StorageMonitor.h                    # High-level storage chamber monitor
│   ├── SystemMonitor.h                     # Linux /proc virtual filesystem telemetry
│   ├── TcpClient.h                         # TCP client that sends one update and waits for the ACK
│   ├── TcpProtocol.h                       # Wire text protocol & FacilityMessage
│   ├── TcpServer.h                         # Multi-client TCP server
│   ├── TemperatureMonitor.h                # Centralized temperature state & history
│   └── ThreadedMonitor.h                   # In-process std::thread concurrency engine
│
├── src/                                    # C++ Implementation files
│   ├── alert_system.cpp                    # Priority alert queue implementation
│   ├── DeviceSensor.cpp                    # POSIX system call wrapper implementation
│   ├── expiry_utils.cpp                    # Calendar difference algorithms
│   ├── inventory_manager.cpp               # Inventory CRUD & file persistence
│   ├── IPCManager.cpp                      # Pipe, shm, and sem implementations
│   ├── main.cpp                            # Interactive 18-option CLI application
│   ├── medicine.cpp                        # Medicine logic & serialization
│   ├── medisave_client.cpp                 # Standalone TCP facility update client
│   ├── medisave_server.cpp                 # Standalone TCP facility coordination hub
│   ├── monitor_worker.cpp                  # Standalone background monitoring executable
│   ├── NetworkManager.cpp                  # High-level network coordinator implementation
│   ├── ProcessManager.cpp                  # fork, exec, and waitpid implementation
│   ├── RedistributionEngine.cpp            # Deterministic surplus/shortage matching
│   ├── StorageMonitor.cpp                  # Chamber condition analysis implementation
│   ├── SystemMonitor.cpp                   # /proc virtual filesystem parsing implementation
│   ├── TcpClient.cpp                       # Socket connect/send/recv implementation
│   ├── TcpServer.cpp                       # Socket bind/listen/accept & worker threads
│   ├── TemperatureMonitor.cpp              # Central temperature telemetry implementation
│   └── ThreadedMonitor.cpp                 # std::thread, mutex, and condition_variable implementation
│
├── driver/                                 # Linux Kernel Module
│   ├── Makefile                            # Kernel module build script
│   ├── medisave_driver.c                   # Character device driver source
│   └── README.md                           # Driver documentation & setup guide
│
├── tests/                                  # Automated Test Suites (9 C++ Suites + 1 Shell Suite)
│   ├── cli_regress.sh                      # Automated CLI regression test suite (5 tests)
│   ├── device_sensor_test.cpp              # 18 automated driver integration tests
│   ├── driver_test.cpp                     # Direct driver verification / userspace fallback test
│   ├── ipc_test.cpp                        # Anonymous pipe, POSIX shared memory, and semaphore unit tests
│   ├── process_test.cpp                    # fork, exec, and waitpid lifecycle tests
│   ├── redistribution_test.cpp             # Surplus/shortage matching and transfer bounds tests
│   ├── sign.ps1                            # Windows Authenticode code signing script for local dev
│   ├── system_monitor_test.cpp             # Linux /proc filesystem parsing tests
│   ├── tcp_test.cpp                        # TCP server, client & protocol unit tests
│   ├── test_inventory.cpp                  # 49 automated inventory unit tests
│   └── thread_test.cpp                     # Multithreading, mutex & CV unit tests
│
├── data/                                   # Data directory
│   └── medicines.txt                       # Persistent inventory storage (25 records)
│
└── docs/                                   # Documentation
    ├── README.md                           # Master documentation index
    ├── project_structure.txt               # Complete repository directory tree
    ├── architecture/                       # Subsystem architecture specifications
    ├── demo/                               # Trainer demo script & command sheets
    ├── progress/                           # Milestone development reports (Stages 1-6)
    │   ├── stage1_inventory.md             # Stage 1 milestone report
    │   ├── stage2_device_driver.md         # Stage 2 milestone report
    │   ├── stage3_driver_integration.md    # Stage 3 milestone report
    │   ├── stage4_process_ipc.md           # Stage 4 milestone report
    │   ├── stage5_multithreading_tcp.md    # Stage 5 milestone report
    │   ├── stage6_final_features.md        # Stage 6 milestone report
    │   └── progress_report.md              # Master consolidated progress report
    ├── requirements/                       # Requirements specifications
    ├── testing/                            # Audit and official test result reports
    └── uml/                                # PlantUML diagrams & documentation
```

---

## 16. Requirements

### Operating System:
* Linux (Ubuntu 20.04+, Debian 11+, or compatible Linux distribution, x86_64).

### Build Toolchain:
* GCC / G++ (supporting C++17, version 9.0 or later).
* GNU Make (version 4.0 or later).
* POSIX Real-time and Threads libraries (`-pthread`, `-lrt`).

### Linux Kernel Headers:
* Matching running kernel (`linux-headers-$(uname -r)`).

---

## 17. Installation

```bash
# 1. Clone repository
git clone https://github.com/Tribhuwansingh2023/MediSave-Edge.git
cd MediSave-Edge

# 2. Install prerequisites (Ubuntu/Debian)
sudo apt update
sudo apt install -y build-essential linux-headers-$(uname -r) make gcc g++

# 3. Compile all applications, workers, and test runners
make all

# 4. Compile Linux Character Device Driver
make driver
```

---

## 18. Running the Project

### 1. Load the Kernel Module (on Linux host with Kernel headers):
```bash
sudo insmod driver/medisave_driver.ko

# Temporary prototype testing permission:
sudo chmod 666 /dev/medisave

# Recommended Production deployment uses a restricted udev rule:
# echo 'KERNEL=="medisave", MODE="0660", GROUP="dialout"' | sudo tee /etc/udev/rules.d/99-medisave.rules

ls -l /dev/medisave
dmesg | tail -n 5
```

> **Security Note on Device Permissions:**  
> For quick prototype testing, temporary device permissions may be used (`sudo chmod 666 /dev/medisave`). A production deployment should use a udev rule with restricted group-based access such as mode 0660.

### 2. Launch Main Interactive CLI:
```bash
./bin/medisave
```

### 3. Launch Standalone TCP Server & Client (Optional Demo):
```bash
# Terminal 1: Launch Facility Server
./bin/medisave_server

# Terminal 2: Report Surplus from Facility-A
./bin/medisave_client Facility-A Paracetamol P2026A 150 SURPLUS

# Terminal 3: Report Shortage from Facility-B
./bin/medisave_client Facility-B Paracetamol P2026A 20 SHORTAGE
```

> **Port 5000 Conflict Warning:**  
> Menu option 12 launches `TcpServer` on port 5000, and `bin/medisave_server` also binds to port 5000. They should not be run simultaneously to prevent socket binding conflicts.

### 4. Unload Kernel Module:
```bash
sudo rmmod medisave_driver
```

---

## 19. Testing

Execute all automated unit and integration test suites:
```bash
make test
```

Official test report: [`docs/testing/test_results.md`](docs/testing/test_results.md)  
Comprehensive audit: [`docs/testing/project_audit.md`](docs/testing/project_audit.md)  
Consolidated Progress Report: [`docs/progress/progress_report.md`](docs/progress/progress_report.md)

### Milestone Progress Reports
* [**Stage 1: Inventory**](docs/progress/stage1_inventory.md) — Core C++ inventory management, medicine data models, and persistence.
* [**Stage 2: Device Driver**](docs/progress/stage2_device_driver.md) — Linux character device driver (`medisave_driver.ko`), VFS operations, and IOCTL interface.
* [**Stage 3: Driver Integration**](docs/progress/stage3_driver_integration.md) — User-space hardware abstraction layer (HAL) and `/dev/medisave` polling.
* [**Stage 4: Process and IPC**](docs/progress/stage4_process_ipc.md) — Dedicated monitor worker process (`fork`/`exec`), anonymous pipes, POSIX shared memory, and semaphores.
* [**Stage 5: Multithreading and TCP**](docs/progress/stage5_multithreading_tcp.md) — Multi-threaded sensor/alert pipeline, thread-safe queues, and TCP socket client/server networking.
* [**Stage 6: Final Features**](docs/progress/stage6_final_features.md) — Redistribution recommendation engine, `/proc` virtual filesystem telemetry, and executive CLI dashboard.
* [**Consolidated Progress Report**](docs/progress/progress_report.md) — Master consolidated progress report covering all 6 stages.

| Test Suite | Focus Area | Status |
| :--- | :--- | :---: |
| `bin/test_inventory` | CRUD operations, calendar expiry, Max-Heap triage, persistence | **49 / 49 PASS** |
| `bin/device_sensor_test` | POSIX system call wrapper, IOCTL control plane, parameter validation | **18 / 18 PASS** |
| `bin/driver_test` | Kernel module character device test / user-space fallback verification | **PASS (Fallback)** |
| `bin/ipc_test` | Anonymous pipe streaming, POSIX shared memory, POSIX semaphores | **PASS** |
| `bin/process_test` | Multi-process fork, exec, waitpid harvesting, zombie prevention | **PASS** |
| `bin/thread_test` | `std::thread`, `std::mutex`, `std::condition_variable` alert triage | **PASS** |
| `bin/tcp_test` | TCP socket server, client connection, wire protocol, and acknowledgment | **PASS** |
| `bin/redistribution_test` | Surplus/shortage matching, transfer limits, deterministic priority | **PASS** |
| `bin/system_monitor_test` | Virtual filesystem `/proc` direct parsing with fallback resilience | **PASS** |
| `tests/cli_regress.sh` | CLI EOF handling, clean shutdown, and pipe delimiter rejection | **5 / 5 PASS** |
| `driver/medisave_driver.ko` | Linux Character Device Driver (Kernel C99) | **SOURCE VERIFIED** *(Live verification pending Linux host)* |

<details>
<summary><strong>View Real Execution Output (make test)</strong></summary>

```text
==========================================
 Running MediSave Edge Unit Tests...
==========================================
========================================
  MEDISAVE EDGE - UNIT TEST SUITE
========================================

 [PASS] Medicine creation with valid parameters
 [PASS] Medicine quantity retrieval
 [PASS] Medicine not low stock when qty > minStock
 [PASS] Medicine validation rejects negative quantity and bad dates
 [PASS] Add unique medicine ID to inventory
 [PASS] Inventory count increases after addition
 [PASS] Duplicate medicine ID correctly rejected
 [PASS] Inventory count unchanged after duplicate rejection
 [PASS] Search medicine by exact ID
 [PASS] Search non-existent ID returns nullptr
 [PASS] Search medicines by name substring matches correct records
 [PASS] Update stock by adding quantity
 [PASS] Update stock by dispensing quantity
 [PASS] Update stock rejects negative quantity
 [PASS] Remove existing medicine by ID
 [PASS] Removed medicine is no longer in inventory
 [PASS] Removing non-existent medicine returns false
 [PASS] Calculate negative days for expired medicine
 [PASS] Classify EXPIRED status
 [PASS] Calculate 3 days remaining until expiry
 [PASS] Classify CRITICAL status for 3 days
 [PASS] Calculate 20 days remaining until expiry
 [PASS] Classify WARNING status for 20 days
 [PASS] Calculate > 30 days for safe medicine
 [PASS] Classify NORMAL status
 [PASS] getExpiredMedicines detects expired records
 [PASS] getExpiringSoonMedicines detects expiring records
 [PASS] getLowStockMedicines detects depleted records
 [PASS] Alert queue generated with active warnings
 [PASS] Priority Queue correctly prioritizes expired medicine at highest triage rank
 [PASS] Priority Queue ranks critical 3-day expiry immediately following expired stock
 [PASS] Save inventory to text file
 [PASS] Load inventory from text file
 [PASS] Loaded inventory has correct count
 [PASS] Deserialized medicine attributes match original values
 [PASS] Atomic saveToFile created backup .bak file
 [PASS] Constructor rejects '|' in medicine name
 [PASS] Constructor rejects control characters in medicine name
 [PASS] Constructor rejects '|' in batch number
 [PASS] Constructor rejects control characters in batch number
 [PASS] setName rejects '|'
 [PASS] setName rejects control characters
 [PASS] setBatchNumber rejects '|'
 [PASS] setBatchNumber rejects control characters
 [PASS] deserialize rejects line with extra pipe delimiters
 [PASS] getAllMedicines sorted alphabetically by ID
 [PASS] getLowStockMedicines sorted alphabetically by ID
 [PASS] getExpiredMedicines sorted by most overdue first
 [PASS] getExpiringSoonMedicines sorted by soonest expiring first

========================================
 TEST RESULTS: 49 / 49 PASSED
========================================
==========================================
 Running Device Sensor Integration Tests...
==========================================
========================================
       DEVICE SENSOR TEST
========================================

 [PASS] DeviceSensor default path is /dev/medisave
 [PASS] Sensor starts in disconnected state
 [PASS] DeviceSensor custom path configured
 [PASS] Graceful failure when device node is unavailable
 [PASS] Meaningful error message populated on open failure
 [PASS] readTemperature rejected when disconnected
 [PASS] setTemperature rejected when disconnected
 [PASS] getStatus rejected when disconnected
 [PASS] Reject NaN temperature
 [PASS] Reject Infinite temperature
 [PASS] Reject out-of-range negative temperature (-100 C)
 [PASS] Reject out-of-range positive temperature (200 C)
 [PASS] StorageMonitor correctly reports device unavailable
 [PASS] StorageMonitor safe default for isCritical when disconnected
 [PASS] Fake device file open succeeds
 [PASS] Fake device first temperature read succeeds (4.50 C)
 [PASS] Fake device repeated read succeeds via lseek rewind
 [PASS] Fake device getStatus succeeds via lseek rewind

========================================
 TEST RESULTS: 18 / 18 PASSED
========================================
==========================================
 Running IPC (Pipe, Shm, Sem) Tests...
==========================================
[PASS] Pipe creation
[PASS] Pipe communication
[PASS] Shared memory creation
[PASS] Shared memory communication
[PASS] Semaphore synchronization
[PASS] IPC cleanup
[PASS] Semaphore timeout (simulated)

All IPC tests passed.
==========================================
 Running Process (fork, exec, waitpid) Tests...
==========================================
[PASS] Process lifecycle
==========================================
 Running Multithreading (std::thread) Tests...
==========================================
[PASS] ThreadedMonitor starts and stops cleanly
[PASS] ThreadedMonitor sensor worker acquires readings
[PASS] ThreadedMonitor alert worker consumes alerts via CV
[PASS] Thread-safe queue bounds and ordering

All thread tests passed.
==========================================
 Running TCP Client/Server Socket Tests...
==========================================
[PASS] Socket compatibility abstraction initialized
[PASS] Server bind and listen on port 5000
[PASS] Single client connect and send
[PASS] Server parses wire message protocol
[PASS] Server responds with ACK
[PASS] Client receives ACK confirmation
[PASS] Multiple sequential client transactions
[PASS] Handled 300 sequential client connections with finished threads joined
[PASS] Server shutdown

All TCP tests passed.
==========================================
 Running Redistribution Engine Tests...
==========================================
[PASS] Surplus detection
[PASS] Shortage detection
[PASS] Facility matching
[PASS] Transfer quantity
[PASS] Multiple facility handling
[PASS] Recommendation ordering
[PASS] Invalid transfer prevention

All redistribution tests passed.
==========================================
 Running Linux /proc System Monitor Tests...
==========================================
[PASS] CPU information parsing
[PASS] CPU utilization calculation
[PASS] Memory calculation
[PASS] /proc/uptime parsing
[PASS] Mock /proc filesystem direct parsing
[PASS] Graceful failure handling

All system monitor tests passed.
==========================================
 Running CLI Regression Tests...
==========================================
[TEST] Immediate EOF (/dev/null) ... PASSED
[TEST] Clean Menu Exit (Option 18) ... PASSED
[TEST] Invalid menu choice followed by exit ... PASSED
[TEST] Pipe delimiter rejection in Name ... PASSED
[TEST] Pipe delimiter rejection in Batch ... PASSED
==========================================
 CLI Regression Tests Passed: 5 / 5
==========================================
```
</details>

---

## 20. Limitations

1. **Configurable Demonstration Thresholds**: The prototype uses configurable temperature thresholds for demonstration. Sample/default thresholds (such as 2.0°C to 8.0°C for refrigerated cold-chain items) are used for the demo and should not be interpreted as universal storage requirements for all medicines.
2. **Simulated Hardware Sensor**: Environmental telemetry is generated within Linux kernel space memory using an internal state variable. Deployment on physical medical refrigerators requires interfacing with real 1-Wire (DS18B20) or I2C sensors.
3. **Advisory Decision Support**: Redistribution proposals are intentionally non-autonomous to respect healthcare regulatory guidelines requiring licensed pharmacist sign-off.
4. **Localhost Socket Networking**: Distributed facility simulation defaults to `127.0.0.1`. Cross-datacenter production deployments require TLS encryption and WAN routing.
5. **Linux Kernel Dependency for Live Driver**: Live module insertion (`insmod`) requires a Linux kernel with kernel headers (`/lib/modules/$(uname -r)/build`). The driver source code is fully implemented and statically verified.
6. **Demonstration Dataset**: Medicine catalog and facility inventories use fictional demonstration datasets.

---

## 21. Future Scope

* **Physical IoT Sensor Drivers**: Integration with real Dallas 1-Wire (`w1_therm`) or I2C thermal sensors via Linux device tree overlays.
* **Encrypted TLS Communication**: Upgrade plaintext TCP sockets to OpenSSL / TLS for HIPAA-compliant encrypted transport.
* **Hardware Push Notifications**: GSM/SMS and webhook alerts dispatched directly to on-call biomedical technicians during critical excursions.
* **Embedded Linux Deployment**: Porting the kernel driver and core engine to lightweight Yocto / Buildroot images on Raspberry Pi or BeagleBone hardware.
* **Cold-Chain GPS Tracking**: Integration with cellular GPS modules for in-transit refrigerated transport tracking.

---

## 22. Author

| Attribute | Details |
| :--- | :--- |
| **Student Name** | Tribhuwan Singh |
| **Email** | webosingh93@gmail.com |
| **GitHub Repository** | [Tribhuwansingh2023/MediSave-Edge](https://github.com/Tribhuwansingh2023/MediSave-Edge) |
| **License** | MIT License |

---

## 23. AI Assistance

AI tools were used for code review, bug fixing and cleanup with trainer approval. The design and implementation are the student's own work.

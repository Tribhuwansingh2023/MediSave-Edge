# MediSave Edge
### Linux-Based Medicine Storage Monitoring, Inventory Alert and Redistribution Decision System

**Student Name:** Tribhuwan Singh  
**Project Type:** Individual Capstone Project  
**Domain:** Software & Hardware Architecture / Linux System Programming  
**Implementation Languages:** C (Linux Kernel Module) / C++17 (User-space Engine)  
**Target Environment:** Linux  

---

## 1. Project Overview

**MediSave Edge** is a Linux-based medicine storage monitoring, inventory alert, and redistribution decision-support prototype implemented using modern C++17 and low-level Linux systems programming concepts.

The system is designed as an operational prototype and decision-support monitoring tool. It continuously tracks pharmaceutical environmental conditions, identifies impending shelf-life expirations, ranks inventory risks using prioritized alert queues, and calculates deterministic redistribution proposals across distributed healthcare facilities.

> **CLINICAL & REGULATORY NOTICE:**  
> MediSave Edge is an advisory decision-support software prototype. It does **not** make autonomous medical diagnoses, establish treatment regimens, or automatically execute physical drug transfers. All redistribution proposals require human review and authorization by certified medical officers and pharmacists.

---

## 2. Problem Statement

Public health facilities and decentralized healthcare depots routinely face critical inventory imbalances:
* **Thermal Spoilage Risks**: Temperature-sensitive pharmaceuticals (e.g., Insulin, Epinephrine, vaccines) require strict cold-chain maintenance (2.0°C to 8.0°C). Undetected cooling failures cause silent efficacy loss.
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
* **Stock Monitoring**: Dynamic threshold validation detecting depleted stock ($qty \le minStock$).
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
* **Transfer Constraints**: Transfers strictly bounded by $\min(\text{Surplus}, \text{Shortage})$; never negative or zero.
* **Advisory Disclaimer**: Software proposals that do not execute automatic physical movements.

### System Monitoring
* **/proc/cpuinfo**: Processor model identification and logical core topology.
* **/proc/stat**: Multi-sample delta calculation for accurate CPU load percentage.
* **/proc/meminfo**: Physical memory metrics (`MemTotal`, `MemAvailable`, memory used percentage).
* **/proc/uptime**: Conversion of kernel uptime seconds into human-readable duration strings.

---

## 5. Technology Stack

* **Programming Languages**: C (Linux Kernel Module, C99) / C++ (User-space Engine, C++17).
* **Operating System**: Linux (Ubuntu 22.04 LTS / Debian 12 / Linux Kernel 5.x–6.x).
* **Build System**: GNU Make, GCC / G++ toolchain.
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
/dev/medisave (Major 240 / alloc_chrdev_region)
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

> **IMPORTANT:**  
> The system generates advisory recommendations only. It does **not** automatically modify local stock, dispatch transport, or execute physical medicine transfers.

---

## 14. Linux System Monitoring (`/proc`)

MediSave Edge directly parses the virtual filesystem without third-party monitoring libraries:
* **/proc/cpuinfo**: Extracts processor model string (`model name` / `Hardware`) and counts logical processor instances.
* **/proc/stat**: Samples cumulative CPU counters across a 50–100ms interval to compute delta utilization percentage:
  $$\text{CPU Usage \%} = \frac{\Delta\text{Total} - \Delta\text{Idle}}{\Delta\text{Total}} \times 100$$
* **/proc/meminfo**: Reads `MemTotal` and `MemAvailable` to compute active memory consumption in gigabytes and percentage.
* **/proc/uptime**: Reads elapsed kernel uptime seconds and formats into duration strings (e.g., `"2 days, 1 hours 15 minutes"`).

---

## 15. Project Structure

```text
MediSave-Edge/
├── Makefile                                # Master build system
├── README.md                               # Complete project documentation
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
│   ├── TcpClient.h                         # Non-blocking TCP client dispatcher
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
├── tests/                                  # Automated Test Suites (8 Suites)
│   ├── device_sensor_test.cpp              # 14 automated driver integration tests
│   ├── driver_test.cpp                     # Direct driver verification
│   ├── ipc_test.cpp                        # Pipe, shm, and semaphore unit tests
│   ├── process_test.cpp                    # fork, exec, and waitpid lifecycle tests
│   ├── redistribution_test.cpp             # 12 redistribution matching & bounds tests
│   ├── system_monitor_test.cpp             # Linux /proc filesystem parsing tests
│   ├── tcp_test.cpp                        # TCP server, client & protocol unit tests
│   ├── test_inventory.cpp                  # 35 automated inventory tests
│   └── thread_test.cpp                     # Multithreading, mutex & CV unit tests
│
├── data/                                   # Data directory
│   └── medicines.txt                       # Persistent inventory storage
│
└── docs/                                   # Documentation
    ├── README.md                           # Master documentation index
    ├── project_structure.txt               # Complete repository directory tree
    ├── architecture/                       # Subsystem architecture specifications
    ├── demo/                               # Trainer demo script & command sheets
    ├── progress/                           # Milestone development reports
    ├── requirements/                       # Requirements specifications
    ├── testing/                            # Audit and official test result reports
    └── uml/                                # PlantUML diagrams & documentation
```

---

## 16. Requirements

### Operating System:
* Linux (Ubuntu 20.04+, Debian 11+, or compatible Linux distribution).
* x86_64 architecture.

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

### 1. Load the Kernel Module:
```bash
sudo insmod driver/medisave_driver.ko
sudo chmod 666 /dev/medisave
ls -l /dev/medisave
dmesg | tail -n 5
```

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

### 4. Unload Kernel Module:
```bash
sudo rmmod medisave_driver
```

---

## 19. Testing

Execute all 8 automated unit and integration test suites:
```bash
make test
```

Official test report: [`docs/testing/test_results.md`](docs/testing/test_results.md)  
Comprehensive audit: [`docs/testing/project_audit.md`](docs/testing/project_audit.md)

| Test Suite | Focus Area | Status |
| :--- | :--- | :---: |
| `bin/test_inventory` | CRUD operations, calendar expiry, Max-Heap triage, persistence | **35 / 35 PASS** |
| `bin/device_sensor_test` | POSIX system call wrapper, IOCTL control plane, parameter validation | **14 / 14 PASS** |
| `bin/driver_test` | Direct kernel module open, read, write, ioctl, and close verification | **PASS** |
| `bin/ipc_test` | Anonymous pipe streaming, POSIX shared memory, POSIX semaphores | **PASS** |
| `bin/process_test` | Multi-process fork, exec, waitpid harvesting, zombie prevention | **PASS** |
| `bin/thread_test` | `std::thread`, `std::mutex`, `std::condition_variable` alert triage | **PASS** |
| `bin/tcp_test` | TCP socket server, client connection, payload exchange, ACK parsing | **PASS** |
| `bin/redistribution_test` | Surplus/shortage matching, transfer limits, deterministic priority | **PASS** |
| `bin/system_monitor_test` | Virtual filesystem `/proc` direct parsing with fallback resilience | **PASS** |

---

## 20. Limitations

1. **Simulated Hardware Sensor**: Environmental telemetry is generated within Linux kernel space memory using an internal state variable. Deployment on physical medical refrigerators requires interfacing with real 1-Wire (DS18B20) or I2C sensors.
2. **Advisory Decision Support**: Redistribution proposals are intentionally non-autonomous to respect healthcare regulatory guidelines requiring licensed pharmacist sign-off.
3. **Localhost Socket Networking**: Distributed facility simulation defaults to `127.0.0.1`. Cross-datacenter production deployments require TLS encryption and WAN routing.
4. **Demonstration Dataset**: Medicine catalog and facility inventories use fictional demonstration datasets.

---

## 21. Future Scope

* **Physical IoT Sensor Drivers**: Integration with real Dallas 1-Wire (`w1_therm`) or I2C thermal sensors via Linux device tree overlays.
* **Encrypted TLS Communication**: Upgrade plaintext TCP sockets to OpenSSL / TLS for HIPAA-compliant encrypted transport.
* **Hardware Push Notifications**: GSM/SMS and webhook alerts dispatched directly to on-call biomedical technicians during critical excursions.
* **Embedded Linux Deployment**: Porting the kernel driver and core engine to lightweight Yocto / Buildroot images on Raspberry Pi or BeagleBone hardware.
* **Cold-Chain GPS Tracking**: Integration with cellular GPS modules for in-transit refrigerated transport tracking.

---

## 22. Author

* **Student Name:** Tribhuwan Singh
* **Email:** webosingh93@gmail.com
* **GitHub Repository:** [Tribhuwansingh2023/MediSave-Edge](https://github.com/Tribhuwansingh2023/MediSave-Edge)
* **License:** MIT License

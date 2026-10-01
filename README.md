# MediSave Edge
### A Linux-Based Medicine Storage Monitoring, Inventory Alert and Redistribution System

**Student Name:** Tribhuwan Singh  
**Project Type:** Individual Capstone Project  
**Domain:** Software & Hardware Architecture / Linux System Programming  
**Implementation Languages:** C (Linux Kernel Module) / C++17 (User-space Engine)  
**Target Environment:** Linux  

---

## 1. Project Overview

**MediSave Edge** is a modular, high-reliability system designed for monitoring medicine storage conditions, analyzing inventory levels, detecting impending expiration dates, raising prioritized triage alerts, and interfacing directly with low-level Linux character device drivers (`/dev/medisave`) to simulate environmental sensory hardware.

The project demonstrates complete vertical integration: from low-level kernel driver code (Ring 0) to application-level C++ data structures, multi-process management, IPC streaming, and terminal-based interactive controls (Ring 3).

---

## 2. Implemented Modules

### 2.1 Linux Character Device Driver (`/dev/medisave`)
* **Loadable Kernel Module (LKM):** Implemented in pure C (`driver/medisave_driver.c`) using modern Linux kernel APIs (`alloc_chrdev_region`, `cdev_init`, `cdev_add`, `class_create`, `device_create`).
* **VFS File Operations:** Implements `open()`, `read()`, `write()`, `unlocked_ioctl()`, and `release()`.
* **Hardware Abstraction:** Simulates physical temperature sensor hardware in kernel memory using fixed-point integer representations (milli-Celsius) in accordance with Linux kernel standards prohibiting FPU registers.
* **Safe Kernel/User Data Transfer:** Uses `copy_to_user()` and `copy_from_user()` to ensure memory isolation and page table verification.
* **IOCTL Control Plane:** Shared binary interface (`include/medisave_ioctl.h`) supporting `MEDISAVE_IOC_SET_TEMP`, `MEDISAVE_IOC_GET_TEMP`, `MEDISAVE_IOC_GET_STATUS`, and `MEDISAVE_IOC_GET_DATA`.
* **Configurable Triage Thresholds:**
  * $< 2.0^\circ\text{C}$: `LOW`
  * $2.0^\circ\text{C} - 8.0^\circ\text{C}$: `NORMAL` (Standard cold chain)
  * $8.1^\circ\text{C} - 10.0^\circ\text{C}$: `WARNING`
  * $> 10.0^\circ\text{C}$: `CRITICAL`
* **Concurrency Protection:** Mutex locking (`DEFINE_MUTEX`) preventing data races during concurrent read/write operations.

### 2.2 Linux Device Driver Integration (`DeviceSensor` & `StorageMonitor`)
* **Hardware Abstraction Layer (`DeviceSensor`):** C++ class (`include/DeviceSensor.h`, `src/DeviceSensor.cpp`) encapsulating low-level POSIX system calls (`open`, `read`, `write`, `ioctl`, `close`) to communicate directly with `/dev/medisave`.
* **Storage Condition Assessment (`StorageMonitor`):** C++ class (`include/StorageMonitor.h`, `src/StorageMonitor.cpp`) converting raw sensor streams into classified condition statuses (`NORMAL`, `WARNING`, `CRITICAL`), formatting terminal displays, and alerting if storage tolerances of active medicine batches are breached.
* **Graceful Degradation:** If `/dev/medisave` is unavailable (e.g. driver not loaded), the system informs the operator cleanly without crashing, allowing inventory operations to continue unhindered.

### 2.3 Linux Processes, IPC & Signal Handling (`ProcessManager` & `IPCManager`)
* **Multi-Process Architecture (`fork()` & `exec()`):** Decouples real-time storage monitoring into a dedicated child process (`bin/monitor_worker`) spawned via `fork()` and initialized with `execl()`.
* **Zombie Process Elimination (`waitpid()`):** The main process inspects worker health and harvests exit codes cleanly (`WIFEXITED`, `WEXITSTATUS`, `WIFSIGNALED`), preventing zombie processes in the OS process table.
* **Anonymous Pipes (`pipe()`):** High-speed streaming IPC channel transferring live temperature and status messages from worker to parent (`TEMP=6.50;STATUS=NORMAL;...`). Unused descriptors are strictly closed in parent and child.
* **POSIX Shared Memory (`shm_open()`, `mmap()`):** Zero-copy shared data structure (`struct SharedMonitorData`) mapped into virtual memory spaces for rapid telemetry queries without kernel socket overhead.
* **POSIX Named Semaphore (`sem_open()`, `sem_wait()`, `sem_post()`):** Mutual exclusion lock (`/medisave_sem_v1`) synchronizing concurrent reads and writes to shared memory to prevent torn reads.
* **POSIX Signal Handling (`sigaction()`):** Clean signal interceptors for `SIGINT` (Ctrl+C), `SIGTERM` (external termination), and `SIGUSR1` (immediate telemetry poll) using `volatile sig_atomic_t` flags to ensure async-signal safety.
* **Deterministic Graceful Teardown:** Orderly termination sequence guaranteeing that worker processes terminate, pipe file descriptors close, and shared memory objects and semaphores are unlinked (`shm_unlink`, `sem_unlink`).

### 2.4 Medicine Domain Model
* Encapsulated `Medicine` class (`include/medicine.h`, `src/medicine.cpp`) modeling:
  * Medicine ID, Name, and Batch Number
  * Current Stock Quantity
  * Expiry Date (`YYYY-MM-DD` format)
  * Minimum Required Stock & Maximum Stock Capacity
  * Storage Temperature Range (Minimum °C and Maximum °C)
* Strict input validation against negative stock, inverted thresholds, and malformed dates.

### 2.5 Inventory Management Engine
* High-performance in-memory inventory organized via `std::unordered_map<std::string, Medicine>` for $O(1)$ average-time lookups by medicine ID.
* CRUD operations, substring search, inventory summary tables, and stock adjustments with underflow protection.

### 2.6 Expiry Monitoring & Triage Analysis
* Calendar calculation engine (`include/expiry_utils.h`, `src/expiry_utils.cpp`) computing signed days remaining relative to system time.
* Categories: `EXPIRED` ($<0\text{d}$), `CRITICAL` ($0-7\text{d}$), `WARNING` ($8-30\text{d}$), and `NORMAL` ($>30\text{d}$).

### 2.7 Priority Alert System
* Evaluates inventory conditions using an STL Max-Heap (`std::priority_queue`).
* Prioritizes: Expired Stock > Critical Expirations ($\le 7\text{d}$) > Storage Chamber Excursions > Depleted Stock > Low Stock > Warning Expirations.

### 2.8 File-Based Persistence
* Automatic persistence to plain text storage at `data/medicines.txt` using pipe-delimited schemas without third-party dependencies.

---

## 3. Project Structure

```text
MediSave-Edge/
├── Makefile                        # Master build system (all, test, ipc-test, process-test)
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
│   ├── StorageMonitor.h            # High-level chamber monitor
│   ├── medisave_ioctl.h            # Shared kernel/user IOCTL definitions
│   ├── SharedData.h                # POSIX shared memory layout
│   ├── IPCManager.h                # Pipe, shared memory, and semaphore manager
│   └── ProcessManager.h            # Process lifecycle (fork/exec/waitpid) manager
│
├── src/                            # C++ Implementation files
│   ├── medicine.cpp                # Medicine logic & serialization
│   ├── inventory_manager.cpp       # Inventory CRUD & persistence
│   ├── expiry_utils.cpp            # Calendar difference algorithms
│   ├── alert_system.cpp            # Heap ordering & alert rendering
│   ├── DeviceSensor.cpp            # POSIX driver system call wrapper
│   ├── StorageMonitor.cpp          # Storage condition analysis
│   ├── IPCManager.cpp              # Anonymous pipe, shm, and sem implementations
│   ├── ProcessManager.cpp          # fork, exec, waitpid, and kill implementation
│   ├── monitor_worker.cpp          # Independent background monitoring executable
│   └── main.cpp                    # Interactive CLI application
│
├── driver/                         # Linux Kernel Module
│   ├── medisave_driver.c           # Character device driver source
│   ├── Makefile                    # Kernel module build script
│   └── README.md                   # Driver documentation & guide
│
├── tests/                          # Automated Test Suites
│   ├── test_inventory.cpp          # 35 automated inventory tests
│   ├── device_sensor_test.cpp      # 14 automated driver integration tests
│   ├── ipc_test.cpp                # Pipe, shm, and semaphore unit tests
│   └── process_test.cpp            # fork, exec, and waitpid lifecycle tests
│
├── data/                           # Data directory
│   └── medicines.txt               # Persistent inventory storage
│
└── docs/                           # Documentation
    ├── architecture/
    │   ├── device_driver_architecture.md # Kernel driver architecture & system calls
    │   ├── cpp_driver_integration.md     # C++ ↔ driver integration design
    │   └── process_ipc_architecture.md   # Multi-process, IPC & signal architecture
    └── progress/
        ├── day1_inventory.md       # Day 1 Inventory milestone report
        ├── day1_device_driver.md   # Day 1 Driver 3-minute demo script
        ├── task4_driver_integration.md # Task 4 integration report
        └── task5_process_ipc.md    # Task 5 process & IPC progress report
```

---

## 4. Build Instructions

### Prerequisites (Ubuntu/Debian):
```bash
sudo apt update
sudo apt install -y build-essential linux-headers-$(uname -r) make gcc g++
```

### Compiling User-Space Applications:
```bash
# Compiles all executables (main app, worker, and all test suites)
make all
```

### Compiling Linux Character Device Driver:
```bash
# Compiles driver/medisave_driver.ko
make driver
```

---

## 5. Usage & Execution Workflow

### 1. Load the Kernel Module:
```bash
cd driver
make
sudo insmod medisave_driver.ko
sudo chmod 666 /dev/medisave
dmesg | tail -n 5
cd ..
```

### 2. Verify Character Device Node:
```bash
ls -l /dev/medisave
# Expected: crw-rw-rw- 1 root root <major>, 0 /dev/medisave
```

### 3. Run Automated Tests:
```bash
# Run all 4 comprehensive test suites:
make test

# Or run specific subsystem tests:
make ipc-test
make process-test
```

### 4. Launch Main Interactive Application:
```bash
./bin/medisave
```

#### Interactive CLI Menu:
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
 9. Save Inventory
10. Read Storage Temperature
11. Set Simulated Temperature
12. Show Storage Condition
13. Start Background Monitoring (fork/exec)
14. Stop Background Monitoring (SIGTERM/waitpid)
15. Show Monitoring Status (IPC/Pipe/Shm)
16. Exit
========================================
```

### 5. Graceful Teardown Demo:
Pressing `Ctrl+C` or selecting Option `16` at any time initiates an immediate, clean shutdown sequence:
```text
========================================
Shutdown requested...
Stopping monitor process...
Cleaning IPC resources...
MediSave Edge stopped safely.
========================================
```

### 6. Unload Driver after Evaluation:
```bash
sudo rmmod medisave_driver
```

---

## 6. Next Milestone (Task 6)

* **POSIX Multithreading:** High-frequency chamber anomaly listener and worker threads.
* **TCP Socket Client/Server:** Inter-facility networking for medicine shortage/surplus query exchange.
* **Redistribution Decision Engine:** Automated matching algorithm transferring near-expiry medicines to shortage centers.

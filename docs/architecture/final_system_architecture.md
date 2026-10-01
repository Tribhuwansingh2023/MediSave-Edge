# MediSave Edge — Final System Architecture Specification

## 1. Overview
**MediSave Edge** is a modular Linux-based prototype combining low-level Linux systems programming, kernel device driver interaction, concurrency, network sockets, and decision support algorithms for pharmaceutical storage assurance and inventory redistribution.

---

## 2. High-Level Architecture

```
                                +-----------------------------+
                                |        MEDISAVE EDGE        |
                                |     C++17 Core Engine       |
                                +-----------------------------+
                                               |
         +-------------------------------------+-------------------------------------+
         |                                     |                                     |
         v                                     v                                     v
+------------------+                 +--------------------+                +-------------------+
|    INVENTORY     |                 | STORAGE MONITORING |                | FACILITY NETWORK  |
|  - CRUD Engine   |                 | - DeviceSensor     |                | - TcpServer       |
|  - Expiry Engine |                 | - TempMonitor      |                | - TcpClient       |
|  - Priority Alert|                 | - /dev/medisave    |                | - FacilityMessage |
+------------------+                 +--------------------+                +-------------------+
         |                                     |                                     |
         +-------------------------------------+-------------------------------------+
                                               |
                                               v
                                     +--------------------+
                                     |    ALERT ENGINE    |
                                     |  - Expiry Alerts   |
                                     |  - Thermal Alerts  |
                                     +--------------------+
                                               |
                                               v
                                     +--------------------+
                                     |   REDISTRIBUTION   |
                                     |  - Stock Classify  |
                                     |  - Match Engine    |
                                     |  - Advisory Output |
                                     +--------------------+
                                               |
                                               v
                                     +--------------------+
                                     |  EXECUTIVE DASHBOARD|
                                     |  - Storage State   |
                                     |  - Inventory State |
                                     |  - Redistribution  |
                                     |  - Host System     |
                                     +--------------------+
                                               |
                         +---------------------+---------------------+
                         |                     |                     |
                         v                     v                     v
                 +---------------+     +---------------+     +---------------+
                 |  CPU /proc    |     | MEMORY /proc  |     | UPTIME /proc  |
                 +---------------+     +---------------+     +---------------+
```

---

## 3. Hardware & Software Architecture
* **Simulated Hardware Layer**: Physical sensor telemetry simulated within the Linux Kernel module memory using fixed-point integer mathematics (milli-Celsius).
* **POSIX VFS Interface**: Hardware exposed via `/dev/medisave` standard character device node.
* **User-Space Software Stack**: Compiled with GCC/G++ C++17, linked against POSIX threads (`-pthread`) and real-time extensions (`-lrt`).

---

## 4. User Space / Kernel Space Separation

```
================================================================================
                               USER SPACE (Ring 3)
================================================================================
  MediSave CLI (main.cpp)
       |
       +---> TemperatureMonitor  --->  DeviceSensor
       |                                    |
       |                                    | open(), read(), write(), ioctl()
=======================================|========================================
       |                               | POSIX System Calls
=======================================v========================================
                               KERNEL SPACE (Ring 0)
================================================================================
  Linux VFS Layer (Virtual File System)
       |
       v
  Character Device Subsystem (/dev/medisave)
       |
       +---> copy_to_user()  /  copy_from_user()
       |
       v
  MediSave Kernel Module (driver/medisave_driver.c)
       |
       +---> Internal State: current_temp_milli (e.g. 6500 = 6.50 °C)
       +---> Concurrency: DEFINE_MUTEX(medisave_mutex)
       +---> IOCTL Control Plane: medisave_ioctl.h
================================================================================
```

---

## 5. Device Driver Architecture
* **Registration**: `alloc_chrdev_region` allocates major/minor device numbers dynamically.
* **CDEV Initialization**: `cdev_init` binds standard file operations:
  * `.open = medisave_open`
  * `.read = medisave_read`
  * `.write = medisave_write`
  * `.unlocked_ioctl = medisave_ioctl`
  * `.release = medisave_release`
* **Device Node Creation**: `class_create` and `device_create` expose `/dev/medisave` under `udev`.
* **Thermal Threshold Invariant**:
  * $< 2.0^\circ\text{C}$: `LOW`
  * $2.0^\circ\text{C} - 8.0^\circ\text{C}$: `NORMAL`
  * $8.1^\circ\text{C} - 10.0^\circ\text{C}$: `WARNING`
  * $> 10.0^\circ\text{C}$: `CRITICAL`

---

## 6. Inventory Architecture
* **Storage Structure**: `std::unordered_map<std::string, Medicine>` ensures $O(1)$ search and update time complexity.
* **Expiry Engine**: `ExpiryUtils::calculateDaysUntilExpiry` calculates real calendar offsets.
* **Alert System**: Max-Heap priority queue (`std::priority_queue<Alert, std::vector<Alert>, AlertComparator>`) prioritizing expired drugs, critical expiries ($\le 7$ days), and thermal excursions.

---

## 7. Storage Monitoring Architecture
* **`DeviceSensor` (HAL)**: Encapsulates VFS file descriptor and binary IOCTL calls.
* **`TemperatureMonitor` (State Hub)**: Thread-safe repository recording current temperature, status, timestamp, and a 50-reading historical telemetry ring buffer.
* **`StorageMonitor` (Chamber Assessment)**: Evaluates live conditions against allowed temperature bands of all stored medicine batches.

---

## 8. Process Architecture

```
                  +--------------------------------+
                  |  Main Application Process      |
                  |  PID: Parent (e.g. 5000)       |
                  +--------------------------------+
                                   |
                                   | fork()
                                   v
                  +--------------------------------+
                  |  Child Process Clone           |
                  |  PID: Child (e.g. 5001)        |
                  +--------------------------------+
                                   |
                                   | execl("bin/monitor_worker", ...)
                                   v
                  +--------------------------------+
                  |  Dedicated Monitor Worker      |
                  |  Image replaced: monitor_worker|
                  +--------------------------------+
                                   |
                                   | waitpid(pid, &status, WNOHANG)
                                   v
                  +--------------------------------+
                  |  Parent harvests child status  |
                  |  Zero Zombie Processes         |
                  +--------------------------------+
```

---

## 9. IPC Architecture

### A. Anonymous Pipe (`pipe()`)
* Streaming unidirectional channel transmitting formatted strings (`TEMP=6.50;STATUS=NORMAL\n`) from `monitor_worker` to the parent process. Unused read/write descriptors are strictly closed.

### B. POSIX Shared Memory (`shm_open()`, `mmap()`)
* Shared segment `/medisave_shm_v1` mapping `struct SharedMonitorData` directly into parent and child address spaces for zero-copy queries.

### C. POSIX Named Semaphore (`sem_open()`)
* Binary mutual exclusion semaphore `/medisave_sem_v1` enforcing critical section guards (`sem_wait`, `sem_post`) to eliminate torn reads on shared memory.

---

## 10. Multithreading Architecture

```
                       +----------------------------------+
                       |   MediSave In-Process Threads    |
                       +----------------------------------+
                                        |
                 +----------------------+----------------------+
                 |                                             |
                 v                                             v
      +----------------------+                      +----------------------+
      | Sensor Worker Thread |                      | Alert Worker Thread  |
      | Samples /dev/medisave|                      | Waits on condition   |
      | every 5 seconds      |                      | variable for alerts  |
      +----------------------+                      +----------------------+
                 |                                             ^
                 | Writes state & pushes alert                 |
                 v                                             |
      +--------------------------------------------------------+
      | Shared State guarded by std::mutex                     |
      | Alert queue signaling via std::condition_variable      |
      +--------------------------------------------------------+
```

---

## 11. TCP Networking Architecture

```
  Facility Client A                          Facility Client B
(e.g., Regional Depot)                     (e.g., District Clinic)
          |                                          |
          | connect(127.0.0.1:5000)                  | connect(127.0.0.1:5000)
          v                                          v
   +-------------------------------------------------------+
   |             MediSave TCP Server (TcpServer)           |
   |              Listening on 127.0.0.1:5000              |
   +-------------------------------------------------------+
          |                                          |
          v                                          v
   [Worker Thread A]                          [Worker Thread B]
   Parses incoming stream                     Parses incoming stream
   Returns ACK|Facility-A                     Returns ACK|Facility-B
          |                                          |
          +--------------------+---------------------+
                               |
                               v
   +-------------------------------------------------------+
   |       In-Memory Facility Update Store (Mutexed)       |
   |   Ingested by Redistribution Decision Engine          |
   +-------------------------------------------------------+
```

---

## 12. Redistribution Architecture
1. **Stock Classification**: Identifies surplus ($qty > minReq$) and shortage ($qty < minReq$).
2. **Deterministic Matching**:
   * Sorts shortage facilities descending by need volume.
   * Prioritizes earliest expiry dates.
   * Matches compatible medicine names, IDs, and batches.
   * Transfer volume: $\min(\text{Surplus}_{\text{source}}, \text{Shortage}_{\text{dest}})$.
3. **Advisory Rule**: Proposals are strictly decision-support outputs and do not automatically execute drug shipments.

---

## 13. System Monitoring Architecture
* **/proc/cpuinfo**: Direct parser reading model string and logical processor count.
* **/proc/stat**: Two-sample delta timer computing accurate CPU utilization percentage:
  $$\text{CPU \%} = \frac{\Delta\text{Total} - \Delta\text{Idle}}{\Delta\text{Total}} \times 100$$
* **/proc/meminfo**: Reads `MemTotal` and `MemAvailable` to compute physical RAM utilization.
* **/proc/uptime**: Formats elapsed kernel seconds into human-readable duration strings.

---

## 14. End-to-End Data Flow
1. Kernel Character Driver initializes simulated chamber hardware at `/dev/medisave`.
2. `DeviceSensor` samples temperature via IOCTL or VFS read.
3. `TemperatureMonitor` categorizes condition (`NORMAL`, `WARNING`, `CRITICAL`).
4. If temperature exceeds thresholds, `AlertSystem` pushes a high-urgency alert to the Max-Heap.
5. Regional facilities dispatch inventory status via TCP client sockets.
6. `TcpServer` buffers remote facility balances in thread-safe memory.
7. Operator invokes **Analyze Redistribution**; `RedistributionEngine` calculates matching proposals.
8. Operator views **System Dashboard**; combines storage status, inventory summary, redistribution metrics, and host `/proc` health.

---

## 15. Error Handling Strategy
* **Missing Driver Node**: Displays descriptive guidance without process termination.
* **Socket Disconnection**: Returns error notification and buffers update locally.
* **Corrupted Inventory Records**: Skips invalid lines, preserves valid data, logs parse warning.
* **Out-of-Range Sensor Values**: Rejects invalid values ($<-50^\circ\text{C}$ or $>100^\circ\text{C}$) before dispatching to kernel.

---

## 16. Graceful Shutdown Sequence
1. Catch `SIGINT` / `SIGTERM` via `sigaction` setting async-safe atomic flag `g_shutdownRequested`.
2. Stop and join in-process monitoring threads (`ThreadedMonitor::stop`).
3. Terminate external child processes cleanly via `SIGTERM` and harvest with `waitpid`.
4. Close server listening socket and client connection handles (`closeSocketFd`).
5. Serialize memory inventory to persistent disk file `data/medicines.txt`.
6. Unlink IPC resources (`shm_unlink`, `sem_unlink`).
7. Disconnect character device file descriptor (`close`).

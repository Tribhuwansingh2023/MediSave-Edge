# MediSave Edge — Trainer Demonstration Script

**Target Duration:** 5–10 Minutes  
**Demonstrator:** Tribhuwan Singh  
**Audience:** Evaluator / Technical Trainer  
**Project:** MediSave Edge — Linux-Based Medicine Storage Monitoring, Inventory Alert and Redistribution Decision System  

> **COMPREHENSIVE RUN & VIVA GUIDE:**  
> For the complete step-by-step evaluation cheat sheet with model answers to teacher viva questions, see [**`../../GUIDE.md`**](../../GUIDE.md).

==================================================
MEDISAVE EDGE — 10-MINUTE TRAINER DEMO TIMELINE
==================================================

### 0:00–0:45 — PROBLEM + OBJECTIVE
**Spoken Presentation:**
> "Good morning / afternoon. Today I am presenting **MediSave Edge**, a Linux-based medicine storage monitoring, inventory alert, and redistribution decision-support prototype.
> In pharmaceutical supply chains, facilities face three critical challenges:
> 1. Undetected temperature excursions in storage chambers that compromise drug efficacy.
> 2. Critical medicines expiring unconsumed in one location while neighboring clinics experience acute stockouts.
> 3. Disconnected manual tracking that delays urgent redistribution decisions.
> Our objective was to develop an integrated Linux C/C++ solution that monitors storage conditions, ranks inventory and expiry risks with priority queues, coordinates facility stock over TCP sockets, and computes deterministic redistribution recommendations as an advisory decision-support system."

---

### 0:45–1:30 — SYSTEM ARCHITECTURE
**Action:** Reference System Architecture diagram ([final_system_architecture.md](../architecture/final_system_architecture.md) / PlantUML diagram):
**Spoken Presentation:**
> "The architecture cleanly separates hardware simulation, kernel space, and user space:
> * **Ring 0 (Kernel Space):** Our custom Linux character device driver exposes `/dev/medisave` through standard VFS operations and an IOCTL control plane. Internal temperature state is represented in integer milli-Celsius without floating point and synchronized with a kernel mutex.
> * **Ring 3 (User Space):** Modern C++17 modular components: `DeviceSensor` interfaces with the device node; `StorageMonitor` tracks cold-chain thresholds; `InventoryManager` handles catalog persistence; `AlertSystem` prioritizes risks via an STL Max-Heap; `TcpServer` and `TcpClient` exchange facility telemetry; and `RedistributionEngine` calculates deterministic transfers.
> * **Host Health:** `SystemMonitor` inspects the Linux virtual `/proc` filesystem directly without external dependencies."

---

### 1:30–2:30 — LINUX DEVICE DRIVER
**Action:** Show driver source and build/load commands (or show source if running in Windows test environment):
```bash
# On a Linux host with kernel headers:
cd driver
make
sudo insmod medisave_driver.ko

# Temporary prototype testing permission:
sudo chmod 666 /dev/medisave
# (In production, a udev rule with mode 0660 and group dialout is used)

ls -l /dev/medisave
dmesg | tail -n 10
cd ..
```
**Spoken Presentation:**
> "Our kernel module `medisave_driver.c` uses `alloc_chrdev_region`, `cdev_init`, `cdev_add`, and `device_create` to register `/dev/medisave`.
> It implements standard file operations: `open()`, `release()`, `read()`, `write()`, and `unlocked_ioctl()`.
> Notice our strict input validation: we parse temperature strings up to 3 decimal places without floating-point math, rejecting malformed text, letters, or out-of-bound values. We protect shared state with `mutex_lock(&sensor_mutex)`.
> For rapid prototype testing, permissions can be temporarily set with `chmod 666 /dev/medisave`, while a production deployment uses a udev rule restricting access to mode 0660 and a dedicated operational group."

---

### 2:30–3:30 — INVENTORY + EXPIRY + ALERTS
**Action:** Launch MediSave CLI:
```bash
./bin/medisave
```
* Select **Option 5 (Display Inventory)**: Show 25 loaded medicine records from `data/medicines.txt`.
* Select **Option 7 (Check Expiry)**: Display expired vs expiring soon breakdown.
* Select **Option 8 (Show Priority Alerts)**: Display priority queue ranking.
**Spoken Presentation:**
> "The system loads inventory into an $O(1)$ `std::unordered_map`.
> Our expiry engine calculates calendar day offsets. Batches with negative remaining days are isolated for quarantine, while batches expiring within 7 days (critical) and 30 days (warning) are flagged.
> In Option 8, our Max-Heap `std::priority_queue` ranks all risks: expired drugs receive highest priority, followed by critical low-stock items and warnings, ensuring store managers address urgent threats first."

---

### 3:30–4:30 — TEMPERATURE MONITORING
**Action:**
* Select **Option 9 (Read Storage Temperature)**: Show current temperature reading.
* Select **Option 10 (Set Simulated Temperature)**: Enter `11.50`.
* Select **Option 11 (Show Storage Condition)**: Show CRITICAL status and affected cold-chain medicines.
**Spoken Presentation:**
> "Through our `DeviceSensor` IOCTL interface, we set the chamber temperature to 11.50 °C.
> Our `StorageMonitor` evaluates the excursion. For demonstration purposes, sample default thresholds (such as 2.0°C to 8.0°C for refrigerated cold-chain items like Insulin and Epinephrine) trigger an immediate CRITICAL alert.
> These thresholds are fully configurable in software and demonstrate how chamber excursions are instantly correlated with specific inventory batches."

---

### 4:30–5:30 — PROCESSES + IPC
**Action:** Explain Linux Process Lifecycle and IPC Architecture:
* Option 12 (Start Background Monitoring) spawns background worker.
* Show `monitor_worker.cpp` and IPC headers.
**Spoken Presentation:**
> "To prevent monitoring stalls from blocking the user interface, MediSave Edge implements Linux multi-process architecture:
> 1. We call `fork()` and `execl()` to launch a decoupled `./bin/monitor_worker` process.
> 2. Telemetry is streamed unidirectionally from worker to parent over an anonymous `pipe()`.
> 3. Zero-copy shared memory (`/medisave_shm_v1`) provides high-speed telemetry access.
> 4. A POSIX named semaphore (`/medisave_sem_v1`) serializes access, preventing torn reads.
> 5. We register `sigaction()` handlers for `SIGINT`, `SIGTERM`, and `SIGUSR1`, and harvest exited children using non-blocking `waitpid(WNOHANG)` to prevent zombie processes."

---

### 5:30–6:30 — THREADS + TCP
**Action:**
* Demonstrate multi-facility message exchange:
  * Select **Option 14 (Send Facility Update)**:
    * Facility: `Facility-A`, Medicine: `Paracetamol`, Batch: `P2026A`, Quantity: `150`, Type: `SURPLUS`
  * Select **Option 14 again**:
    * Facility: `Facility-B`, Medicine: `Paracetamol`, Batch: `P2026A`, Quantity: `20`, Type: `SHORTAGE`
**Spoken Presentation:**
> "In-process concurrency uses `std::thread`, `std::mutex`, and `std::condition_variable` to decouple sensor polling from alert consumption without CPU busy-waiting.
> For distributed facility networking, `TcpServer` listens on port 5000 and spawns detached worker threads per client connection.
> Sockets exchange pipe-delimited messages (`FACILITY|MEDICINE|BATCH|QUANTITY|TYPE`).
> We recently hardened this pipeline:
> - Strict 5-field validation rejects empty names, zero/negative quantities, and malformed types.
> - The shutdown sequence uses `shutdown(serverSocket, SHUT_RDWR)` with non-blocking select timeouts and atomic flags, ensuring thread termination without hanging or socket leaks."

---

### 6:30–7:30 — REDISTRIBUTION RECOMMENDATION
**Action:** Select **Option 15 (Analyze Redistribution)**.
**Spoken Presentation:**
> "The `RedistributionEngine` aggregates local and remote facility stock levels:
> 1. It identifies that Facility-A has 100 surplus units of Paracetamol while Facility-B faces a 30-unit shortage.
> 2. It calculates an optimal suggested transfer: exactly 30 units from Facility-A to Facility-B.
> 3. Transfers are strictly bounded by $\min(\text{Surplus}, \text{Shortage})$ and prioritize largest deficits and earliest expiries first.
> Notice our clear advisory notice: this is an automated decision-support proposal. It does not execute physical drug transfers without authorization by licensed pharmacists."

---

### 7:30–8:30 — /proc SYSTEM MONITORING
**Action:**
* Select **Option 16 (Show System Health)**: CPU cores, RAM utilization, kernel uptime.
* Select **Option 17 (Show System Dashboard)**: View full executive dashboard.
**Spoken Presentation:**
> "The `SystemMonitor` component directly parses Linux virtual files:
> - `/proc/cpuinfo` for processor model and core topology.
> - `/proc/stat` with dual-sample delta math for exact CPU utilization percentage.
> - `/proc/meminfo` for memory total, available, and percentage in use.
> - `/proc/uptime` for system uptime formatting.
> Option 17 integrates live chamber temperature, inventory status, active alerts, redistribution recommendations, and host hardware health into one consolidated dashboard."

---

### 8:30–9:30 — TESTING + GITHUB + UML
**Action:** Exit CLI (Option 18), show GitHub repo, UML diagram, and run automated test suite:
```bash
make test
```
**Spoken Presentation:**
> "The project is completely version-controlled on GitHub with clean commit milestones.
> Architecture is documented using PlantUML class and sequence diagrams.
> Our automated test suite runs 8 test runners:
> - `test_inventory`: 35 unit assertions for CRUD, expiry, and Max-Heap priority queues.
> - `device_sensor_test`: 14 assertions for system call wrappers and IOCTL commands.
> - `driver_test`: Character device interface verification.
> - `ipc_test`: Anonymous pipe, shared memory, and semaphore synchronization.
> - `process_test`: Fork/exec lifecycle and waitpid harvesting.
> - `thread_test`: Multithreaded condition-variable producer-consumer triage.
> - `tcp_test`: Socket lifecycle, 10 validation test cases, clean atomic shutdown.
> - `redistribution_test` & `system_monitor_test`: Transfer bounds and `/proc` parsing.
> All automated tests pass with 100% assertions satisfied."

---

### 9:30–10:00 — CONCLUSION & VIVA READINESS
**Spoken Presentation:**
> "In conclusion, MediSave Edge brings together:
> - Linux Kernel Device Drivers (C, Ring 0, VFS, IOCTL, character devices)
> - Modern Object-Oriented C++17 (STL, memory safety, RAII)
> - POSIX System Programming (Processes, Pipes, Shared Memory, Semaphores, Signals)
> - Concurrency & Sockets (Threads, Mutexes, Condition Variables, TCP/IP)
> - System Monitoring (`/proc` virtual filesystem)
> into a single, cohesive, and explainable capstone project.
> Thank you. I am ready for questions and technical discussion."

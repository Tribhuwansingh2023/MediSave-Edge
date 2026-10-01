# MediSave Edge — Trainer Demonstration Script

**Target Duration:** 5–10 Minutes  
**Demonstrator:** Tribhuwan Singh  
**Audience:** Evaluator / Technical Trainer  

========================================
MEDISAVE EDGE — TRAINER DEMO TIMELINE
========================================

### 0:00–0:40 — INTRODUCTION
**Spoken Presentation:**
> "Good morning / afternoon. Today I am presenting **MediSave Edge**, a Linux-based medicine storage monitoring, inventory alert, and redistribution decision-support prototype.
> This system demonstrates end-to-end Linux systems programming in C and C++17, spanning low-level Linux character device drivers in Ring 0, multi-process lifecycle management with IPC, in-process multithreading, concurrent TCP networking, and direct host telemetry via `/proc`."

---

### 0:40–1:20 — PROBLEM STATEMENT
**Spoken Presentation:**
> "In public health supply chains, pharmaceutical inventory frequently encounters three simultaneous vulnerabilities:
> 1. Unmonitored storage conditions leading to thermal spoilage of cold-chain drugs like Insulin.
> 2. Critical medicines expiring unconsumed in one warehouse while neighboring clinics experience acute stockouts.
> 3. Disconnected manual tracking that delays urgent redistribution decisions.
> MediSave Edge was designed to automate continuous environmental monitoring, rank expiry risks, and formulate deterministic redistribution recommendations."

---

### 1:20–2:00 — ARCHITECTURAL OVERVIEW
**Action:** Open [`docs/architecture/final_system_architecture.md`](../architecture/final_system_architecture.md)
**Spoken Presentation:**
> "The architecture cleanly separates security boundaries:
> * In **Kernel Space**, our character device driver `/dev/medisave` simulates hardware sensors using integer milli-Celsius math and protects internal state with kernel mutexes.
> * In **User Space**, modern C++ components interact through POSIX system calls. Telemetry flows from the driver into `TemperatureMonitor`, feeding our Max-Heap `AlertSystem` and `RedistributionEngine`, which are then surfaced in a single-screen executive dashboard."

---

### 2:00–2:45 — GITHUB & MODULAR CODEBASE
**Action:** Display GitHub repository and project tree:
```bash
tree -L 2
```
**Spoken Presentation:**
> "The repository follows strict modular conventions:
> * `driver/` houses the loadable kernel module source.
> * `include/` and `src/` contain decoupled C++ domain managers.
> * `tests/` contains 8 comprehensive unit and integration test suites.
> * `data/` manages persistent flat-file storage."

---

### 2:45–3:30 — COMPILATION & BUILD SYSTEM
**Action:** Execute clean build from the root directory:
```bash
make clean && make all
```
**Spoken Presentation:**
> "The master Makefile compiles our core application `bin/medisave`, standalone server `bin/medisave_server`, client `bin/medisave_client`, worker `bin/monitor_worker`, and all test binaries under `-Wall -Wextra -std=c++17 -pthread` with zero compiler warnings and zero errors."

---

### 3:30–4:15 — LINUX CHARACTER DEVICE DRIVER
**Action:** Insert kernel module and verify device node:
```bash
cd driver && make && sudo insmod medisave_driver.ko && sudo chmod 666 /dev/medisave && cd ..
ls -l /dev/medisave
dmesg | tail -n 5
```
**Spoken Presentation:**
> "Our driver registers dynamically using `alloc_chrdev_region` and exposes `/dev/medisave`. It implements standard file operations (`open`, `read`, `write`, `unlocked_ioctl`, `release`). We can see the kernel log confirming successful major/minor allocation."

---

### 4:15–5:00 — INVENTORY & EXPIRY TRIAGE
**Action:** Launch MediSave CLI:
```bash
./bin/medisave
```
* Select **Option 5 (Display Inventory)**: Show loaded medicine records.
* Select **Option 7 (Check Expiry)**: Display expired vs expiring soon breakdown.
* Select **Option 8 (Show Alerts)**: Display priority queue ranking.
**Spoken Presentation:**
> "The application loads `data/medicines.txt` into an $O(1)$ `std::unordered_map`. Our expiry engine evaluates current date offsets, immediately classifying expired batches for quarantine and sorting items expiring within 7 days to the top of an STL Max-Heap priority queue."

---

### 5:00–5:45 — TEMPERATURE EXCURSION ALERT
**Action:**
* Select **Option 9 (Read Storage Temperature)**: Show initial 6.50 °C (NORMAL).
* Select **Option 10 (Set Simulated Temperature)**: Enter `11.50`.
* Select **Option 11 (Show Storage Condition)**: Show CRITICAL status and breached medicines (Insulin, Epinephrine).
**Spoken Presentation:**
> "Through our binary IOCTL interface, we set the simulated temperature to 11.50 °C. The driver evaluates this as `CRITICAL`, and the user space `TemperatureMonitor` flags an urgent alert because cold-chain tolerances for Insulin and Epinephrine have been breached."

---

### 5:45–6:30 — LINUX SYSTEM PROGRAMMING (PROCESSES & IPC)
**Action:** Briefly explain the underlying multi-process architecture:
* Option 12 (Start Background Monitoring) spawns threads and process.
**Spoken Presentation:**
> "MediSave Edge utilizes `fork()` and `execl()` to decouple monitoring into a separate `monitor_worker` process. Data is streamed back using anonymous pipes, while zero-copy shared memory is synchronized using POSIX named semaphores to eliminate torn reads. Async signals `SIGINT`, `SIGTERM`, and `SIGUSR1` are safely handled via `sigaction()`."

---

### 6:30–7:15 — TCP MULTI-FACILITY NETWORKING
**Action:**
* Select **Option 14 (Send Facility Update)**:
  * Facility: `Facility-A`, Medicine: `Paracetamol`, Batch: `P2026A`, Qty: `150`, Type: `1 (SURPLUS)`
* Select **Option 14 again**:
  * Facility: `Facility-B`, Medicine: `Paracetamol`, Batch: `P2026A`, Qty: `20`, Type: `2 (SHORTAGE)`
**Spoken Presentation:**
> "Our concurrent TCP server and client communicate over localhost port 5000 using a pipe-delimited protocol. The server acknowledges receipt with `ACK|Facility-A`, buffering facility stock levels in thread-safe memory."

---

### 7:15–8:00 — REDISTRIBUTION DECISION ENGINE
**Action:** Select **Option 15 (Analyze Redistribution)**.
**Spoken Presentation:**
> "The Redistribution Engine matches surplus facilities with shortage facilities. Here, Facility-A has 100 surplus units while Facility-B requires 30 units. The engine calculates an optimal suggested transfer of 30 units. Notice our explicit advisory disclaimer: the system provides decision-support output and does not automatically execute physical drug transfers."

---

### 8:00–8:40 — LINUX /proc HOST MONITORING & DASHBOARD
**Action:**
* Select **Option 16 (Show System Health)**: Show CPU cores, memory GB, uptime.
* Select **Option 17 (Show System Dashboard)**: Show consolidated executive summary.
**Spoken Presentation:**
> "Our `SystemMonitor` parses `/proc/cpuinfo`, `/proc/stat`, `/proc/meminfo`, and `/proc/uptime` with zero third-party libraries. In Option 17, our Executive Dashboard consolidates chamber temperature, inventory counts, redistribution proposals, and host CPU/memory utilization into a single view."

---

### 8:40–9:20 — TEST SUITE VERIFICATION
**Action:** Exit CLI (Option 18) and run test suite:
```bash
make test
```
**Spoken Presentation:**
> "MediSave Edge includes 8 automated test suites covering 35 inventory tests, 14 driver tests, IPC, processes, multithreading, TCP networking, redistribution logic, and `/proc` parsing. All 8 test suites pass with a 100% pass rate."

---

### 9:20–10:00 — CONCLUSION
**Spoken Presentation:**
> "To conclude, MediSave Edge demonstrates complete full-stack Linux engineering: bridging a real Linux kernel character device driver, robust C++17 domain logic, inter-process communication, multi-threading, networking, and systems monitoring into a stable, trainer-ready prototype.
> Thank you. I am ready for questions and viva discussion."

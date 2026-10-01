# MediSave Edge — Local Execution & Trainer Demonstration Guide

**Project Name:** MediSave Edge: Linux-Based Medicine Storage Monitoring, Inventory Alert and Redistribution Decision System  
**Author:** Tribhuwan Singh  
**Capstone Project:** Individual Wipro COE Capstone Project  
**Target Platform:** Linux (POSIX User-Space C++17 / Ring 0 C99 Character Device Driver)  
**Quick Reference Document:** Comprehensive Step-by-Step Local Run & Live Viva Defense Guide  

---

## Table of Contents
1. [Overview & Quick Reference](#1-overview--quick-reference)
2. [Prerequisites & Environment Setup](#2-prerequisites--environment-setup)
   - [Linux Environment (Ubuntu 22.04 LTS / Debian 12)](#a-linux-environment-target-for-full-kernel-evaluation)
   - [Windows Environment (MinGW-w64 / w64devkit Fallback)](#b-windows-environment-local-development--user-space-testing)
3. [Step-by-Step Local Build & Verification](#3-step-by-step-local-build--verification)
4. [Step-by-Step Automated Test Execution](#4-step-by-step-automated-test-execution)
5. [Live Teacher Demonstration Script (What to Show in Front of Trainer)](#5-live-teacher-demonstration-script)
   - [Phase 1: Show Codebase & Build Cleanliness (1 Minute)](#phase-1-codebase-architecture--clean-compilation)
   - [Phase 2: Automated Test Verification (1 Minute)](#phase-2-automated-test-verification)
   - [Phase 3: Linux Character Device Driver (2 Minutes)](#phase-3-linux-character-device-driver-demonstration)
   - [Phase 4: Inventory, Expiry Triage & Max-Heap Alerts (2 Minutes)](#phase-4-inventory-expiry--max-heap-alert-queue)
   - [Phase 5: Temperature Excursion & Cold-Chain Alert (1 Minute)](#phase-5-temperature-excursion--cold-chain-breach)
   - [Phase 6: Multi-Process Lifecycle & IPC (1 Minute)](#phase-6-multi-process-lifecycle--ipc)
   - [Phase 7: Multi-Facility TCP Sockets & Redistribution Engine (1.5 Minutes)](#phase-7-multi-facility-tcp-networking--redistribution-engine)
   - [Phase 8: /proc System Monitoring & Executive Dashboard (1 Minute)](#phase-8-proc-system-monitoring--executive-dashboard)
6. [Trainer Viva Defense: Top 10 Questions & Model Answers](#6-trainer-viva-defense-top-10-questions--model-answers)
7. [Emergency Troubleshooting Cheat-Sheet](#7-emergency-troubleshooting-cheat-sheet)

---

## 1. Overview & Quick Reference

This guide gives you an exact, step-by-step procedure to run **MediSave Edge** on your local machine and conduct a live, high-impact demonstration in front of your trainer or evaluator.

### Core Key Facts at a Glance:
* **Languages:** C++17 for the application and systems layers; C99 for the Linux kernel character device driver.
* **Driver Node:** `/dev/medisave` (dynamic major number, character device, fixed-point integer milli-Celsius representation, kernel mutex synchronization).
* **Inventory Size:** 25 realistic medicines in `data/medicines.txt` across cold-chain (2°C–8°C) and room-temperature (15°C–25°C) categories.
* **IPC Subsystems:** Anonymous pipes (`pipe()`), POSIX shared memory (`/medisave_shm_v1`), POSIX named semaphores (`/medisave_sem_v1`), POSIX signals (`SIGINT`, `SIGTERM`, `SIGUSR1`).
* **Concurrency & Networking:** In-process `std::thread`, `std::mutex`, `std::condition_variable`; distributed multi-client TCP server and client over port 5000.
* **Redistribution:** Deterministic matching algorithm bounded strictly by $\min(\text{Surplus}, \text{Shortage})$.
* **Hardware Telemetry:** Direct parsing of `/proc/cpuinfo`, `/proc/stat`, `/proc/meminfo`, and `/proc/uptime`.

---

## 2. Prerequisites & Environment Setup

### A. Linux Environment (Target for Full Kernel Evaluation)
Recommended for complete Ring 0 kernel module insertion and evaluation (Ubuntu 22.04 LTS, Debian 12, or VMware / VirtualBox Linux VM).

```bash
# 1. Update package manager
sudo apt update

# 2. Install essential compilers, build tools, and kernel headers
sudo apt install -y build-essential linux-headers-$(uname -r) make gcc g++ git
```

### B. Windows Environment (Local Development & User-Space Testing)
If running on Windows using `w64devkit` (MinGW-w64 GCC 16.2.0 + GNU Make 4.4.1):
* All 12 user-space C++ modules, shims, multithreading, socket networking, and `/proc` telemetry compile and run natively.
* The Linux kernel driver source code (`driver/medisave_driver.c`) is statically verified; live `insmod` requires a Linux host.

---

## 3. Step-by-Step Local Build & Verification

Open your terminal in the project root directory:

```bash
# Navigate to project repository
cd MediSave-Edge

# Step 1: Clean any previous build artifacts
make clean

# Step 2: Compile the entire system (Application, Server, Client, Worker, and 8 Test Suites)
make all
```

**Expected Terminal Output:**
* All object files compiled into `build/`.
* All 12 executables linked into `bin/`.
* **Zero compiler warnings (`-Wall -Wextra`) and zero errors.**

To inspect the generated binaries:
```bash
ls -la bin/
```

You should see:
- `medisave` (Main Interactive Application)
- `medisave_server` (Standalone TCP Facility Server)
- `medisave_client` (Standalone TCP Facility Client)
- `monitor_worker` (Decoupled Background Monitoring Process)
- `test_inventory`, `device_sensor_test`, `driver_test`, `ipc_test`, `process_test`, `thread_test`, `tcp_test`, `redistribution_test`, `system_monitor_test`

---

## 4. Step-by-Step Automated Test Execution

Run the master automated test command:

```bash
make test
```

### What Happens During `make test`:
1. `bin/test_inventory`: Runs 35 unit assertions for CRUD, calendar date calculations, and priority queue ordering (**35 / 35 PASS**).
2. `bin/device_sensor_test`: Tests device node abstraction, error guards, and input range validation (**14 / 14 PASS**).
3. `bin/driver_test`: Tests character device communication and graceful user-space fallback (**PASS**).
4. `bin/ipc_test`: Tests anonymous pipe streaming, POSIX shared memory, and semaphore synchronization (**PASS**).
5. `bin/process_test`: Tests `fork()`, child process execution, and `waitpid()` zombie harvesting (**PASS**).
6. `bin/thread_test`: Tests sensor acquisition thread, mutex locking, and condition-variable alert wakeups (**PASS**).
7. `bin/tcp_test`: Tests TCP server startup, client connection, payload exchange, 10 protocol validation cases, and clean atomic shutdown (**PASS**).
8. `bin/redistribution_test`: Tests surplus/shortage matching, transfer bounds, and priority ordering (**PASS**).
9. `bin/system_monitor_test`: Tests `/proc` virtual filesystem parsing for CPU, RAM, and Uptime (**PASS**).

**Result to Point Out to Trainer:**
> "All 8 automated test suites pass with 100% assertions satisfied, confirming stability across domain logic, systems programming, and networking."

---

## 5. Live Teacher Demonstration Script

Follow this sequence when presenting to the evaluator. It follows a structured, logical narrative that takes approximately 5–8 minutes.

### Phase 1: Codebase Architecture & Clean Compilation
1. **Show repository tree:**
   ```bash
   ls -la
   ```
   *Explain:*
   > "Sir/Ma'am, this is MediSave Edge. The project is organized into modular directories: `driver/` for the Linux kernel character device, `include/` and `src/` for user-space C++ modules, `tests/` for unit tests, `data/` for persistent inventory, and `docs/` for architecture and UML models."

2. **Run clean build:**
   ```bash
   make clean && make all
   ```
   *Explain:*
   > "We compile using `-Wall -Wextra -std=c++17 -pthread`. As you can see, there are zero warnings and zero compilation errors."

---

### Phase 2: Automated Test Verification
1. **Run test suite:**
   ```bash
   make test
   ```
   *Explain:*
   > "Before running the interactive application, we execute our 8 test suites. Notice the 35 inventory tests, 14 sensor tests, IPC tests, process lifecycle, multithreading, and 10 TCP protocol validation cases all pass."

---

### Phase 3: Linux Character Device Driver Demonstration
*(On Linux host with kernel headers)*:

1. **Compile and load the kernel module:**
   ```bash
   cd driver
   make
   sudo insmod medisave_driver.ko

   # Temporary prototype access permission:
   sudo chmod 666 /dev/medisave

   # Verify device node:
   ls -l /dev/medisave

   # Check kernel log:
   dmesg | tail -n 10
   cd ..
   ```

   *Explain:*
   > "Here we load `medisave_driver.ko` into Ring 0 kernel space. The driver dynamically allocates a major number using `alloc_chrdev_region`, creates the device class and device node `/dev/medisave`, and synchronizes state using a kernel mutex.
   > Notice our fixed-point math: temperature is represented in integer milli-Celsius because the Linux kernel prohibits floating-point operations.
   > For rapid prototype testing, we temporarily used `chmod 666 /dev/medisave`. For production, we have documented a restricted udev rule with mode 0660 and a dedicated operational group."

*(If presenting on Windows/MinGW fallback)*:
> "On this development workstation without native Linux kernel headers, the driver source code is statically verified in `driver/medisave_driver.c`, and our user-space HAL in `DeviceSensor` gracefully falls back to simulation mode without crashing."

---

### Phase 4: Inventory, Expiry & Max-Heap Alert Queue
1. **Launch MediSave Edge:**
   ```bash
   ./bin/medisave
   ```
2. **Display Inventory Table (Option 5):**
   * Press `5` and `<Enter>`.
   * *Show:* The 25 loaded medicines from `data/medicines.txt`. Point out cold-chain items (Insulin, Vaccines, Epinephrine) with 2.0°C–8.0°C limits vs room-temperature items (Paracetamol, Amoxicillin) with 15.0°C–25.0°C limits.
3. **Check Expiry Report (Option 7):**
   * Press `7` and `<Enter>`.
   * *Show:* The engine calculates current date offsets:
     - **Expired:** Isolated for quarantine (e.g. `MED-003 Insulin`, `MED-011 Oxytocin`, `MED-021 Hydrocortisone`).
     - **Critical ($\le 7$ days):** Flagged for urgent consumption (e.g. `MED-001 Paracetamol`, `MED-006 Epinephrine`).
     - **Warning ($8 \le \text{days} \le 30$):** Flagged for planned re-allocation.
4. **View Priority Alert Queue (Option 8):**
   * Press `8` and `<Enter>`.
   * *Explain:*
     > "We implement an STL Max-Heap (`std::priority_queue`) to prioritize risks. Expired drugs receive highest triage severity, followed by critical impending expiries and low-stock shortages, ensuring store managers address urgent threats first."

---

### Phase 5: Temperature Excursion & Cold-Chain Breach
1. **Read Current Storage Temperature (Option 9):**
   * Press `9` and `<Enter>`. Displays current temperature (e.g. `6.50 °C - NORMAL`).
2. **Simulate Temperature Excursion (Option 10):**
   * Press `10` and `<Enter>`.
   * Enter temperature: `11.50`.
3. **Show Storage Condition (Option 11):**
   * Press `11` and `<Enter>`.
   * *Explain:*
     > "Through our `DeviceSensor` IOCTL interface, we set the chamber temperature to 11.50 °C. The driver evaluates this as `CRITICAL`, and `StorageMonitor` immediately flags breached cold-chain medicines (Insulin, Epinephrine, Vaccines).
     > Sample thresholds (2°C–8°C) are configurable defaults used for demonstration."

---

### Phase 6: Multi-Process Lifecycle & IPC
1. **Explain the Architecture (or Show Menu Option 12):**
   * Press `12` and `<Enter>` to start background monitoring.
   * *Explain:*
     > "To prevent monitoring from stalling the user interface, MediSave Edge decouples telemetry using `fork()` and `execl()` into `./bin/monitor_worker`.
     > Telemetry streams unidirectionally through an anonymous `pipe()`.
     > Zero-copy shared memory (`/medisave_shm_v1`) is locked with a POSIX named semaphore (`/medisave_sem_v1`) to prevent torn reads.
     > We catch asynchronous signals (`SIGINT`, `SIGTERM`, `SIGUSR1`) using `sigaction()`, and harvest child exits using `waitpid(WNOHANG)` to prevent zombie processes."

---

### Phase 7: Multi-Facility TCP Networking & Redistribution Engine
1. **Send Facility Updates (Option 14):**
   * Press `14` and `<Enter>`.
     - Facility Name: `Facility-A`
     - Medicine Name: `Paracetamol`
     - Batch Number: `P2026A`
     - Quantity: `150`
     - Type: `1` (SURPLUS)
   * The server receives the payload, validates it, and replies with `ACK|Facility-A`.
2. **Send Second Facility Update (Option 14):**
   * Press `14` and `<Enter>`.
     - Facility Name: `Facility-B`
     - Medicine Name: `Paracetamol`
     - Batch Number: `P2026A`
     - Quantity: `20`
     - Type: `2` (SHORTAGE)
   * Server validates and replies with `ACK|Facility-B`.
3. **Analyze Redistribution (Option 15):**
   * Press `15` and `<Enter>`.
   * *Show:* The Redistribution Proposal:
     ```text
     [RECOMMENDED TRANSFER]
     Medicine : Paracetamol
     From     : Facility-A (Surplus: 100)
     To       : Facility-B (Deficit: 30)
     Transfer : 30 units
     Priority : Level 1
     Notice   : Advisory decision-support output. Physical transfer requires pharmacist sign-off.
     ```
   * *Explain:*
     > "Our redistribution algorithm deterministically matches regional surpluses with shortages. The transfer is strictly bounded by $\min(\text{Surplus}, \text{Shortage})$.
     > Notice our regulatory compliance disclaimer: the system is an advisory decision-support tool and does not execute automated physical drug movements without human pharmacist authorization."

---

### Phase 8: /proc System Monitoring & Executive Dashboard
1. **Show Host System Health (Option 16):**
   * Press `16` and `<Enter>`.
   * *Show:* CPU Model, Logical Core Count, Multi-sample CPU Utilization %, RAM Usage, and Kernel Uptime.
   * *Explain:*
     > "Our `SystemMonitor` parses `/proc/cpuinfo`, `/proc/stat`, `/proc/meminfo`, and `/proc/uptime` directly with zero external libraries."
2. **Show Executive Dashboard (Option 17):**
   * Press `17` and `<Enter>`.
   * *Show:* Consolidated single-screen view displaying Storage Condition, Inventory Triage, Active Alerts, Redistribution Proposals, and Host Hardware Health.
3. **Graceful Exit (Option 18):**
   * Press `18` and `<Enter>`.
   * *Show:* Clean termination message. The application joins threads, shuts down sockets, stops background worker processes, flushes inventory to disk, and exits cleanly with return code 0.

---

## 6. Trainer Viva Defense: Top 10 Questions & Model Answers

### Q1: Why did you implement a Linux Character Device Driver instead of simply reading from a file or mock sensor?
> **Model Answer:**  
> "A character device driver exposes hardware directly via standard Linux Virtual File System (VFS) operations (`open`, `read`, `write`, `unlocked_ioctl`, `release`). This enforces the separation of privilege between Ring 0 (Kernel Space) and Ring 3 (User Space). In a real pharmaceutical monitoring deployment, this driver communicates over I2C, SPI, or 1-Wire with physical temperature probe microcontrollers. Developing a custom driver demonstrates device number allocation (`alloc_chrdev_region`), `cdev` management, device node creation, and `copy_to_user`/`copy_from_user` boundary safety."

### Q2: Why is temperature stored and calculated in integer milli-Celsius in the driver rather than float?
> **Model Answer:**  
> "Linux kernel space strictly prohibits hardware floating-point operations. The kernel does not save or restore FPU/SSE/AVX vector registers during user-kernel context switches for performance reasons. Attempting floating-point math in kernel space causes compilation errors or undefined kernel panics. Therefore, we use fixed-point arithmetic where $1^\circ\text{C} = 1000\,\text{mC}$. A reading of $6.50^\circ\text{C}$ is represented as $6500\,\text{mC}$, giving thousandth-degree precision using fast 32-bit integer arithmetic."

### Q3: What is the purpose of the IOCTL interface in your driver?
> **Model Answer:**  
> "While standard VFS `read()` and `write()` transfer byte streams, `ioctl()` (Input/Output Control) provides a structured control plane. We defined type-safe binary IOCTL commands (`MEDISAVE_IOC_SET_TEMP`, `MEDISAVE_IOC_GET_TEMP`, `MEDISAVE_IOC_GET_STATUS`, `MEDISAVE_IOC_GET_DATA`) using the Linux `_IOR` and `_IOW` macros. This allows our C++ `DeviceSensor` to exchange structured telemetry directly in binary form with zero string-parsing overhead."

### Q4: How does your process architecture prevent zombie processes?
> **Model Answer:**  
> "When the parent process forks the `monitor_worker` child, it tracks the child PID. During shutdown or status checks, the parent calls `waitpid(childPid, &status, WNOHANG)`. This reaps the child's exit status code from the Linux process table immediately upon termination, preventing it from remaining in a `Z` (Zombie/Defunct) state."

### Q5: What IPC mechanisms are used, and why did you use both pipes and shared memory?
> **Model Answer:**  
> "We use anonymous pipes for unidirectional streaming of periodic telemetry logs from the worker to the parent. We use POSIX Shared Memory (`shm_open`, `mmap`) for instant zero-copy access to the latest sensor state. To prevent race conditions and torn reads between the writing child and reading parent, we synchronize shared memory access using a POSIX Named Semaphore (`sem_open`, `sem_wait`, `sem_post`)."

### Q6: How does the in-process multithreading synchronization work?
> **Model Answer:**  
> "We decouple sensor acquisition from alert processing using two threads:
> 1. A Producer thread polls the sensor at periodic intervals.
> 2. A Consumer thread handles alert evaluation.
> To prevent CPU busy-waiting, the consumer thread sleeps on an `std::condition_variable`. When the producer detects a temperature excursion, it pushes the alert into a queue protected by `std::mutex` and notifies the condition variable (`cv.notify_one()`). This provides event-driven processing with minimal CPU overhead."

### Q7: Why did you modify `TcpServer` to use `select()` with a timeout instead of a blocking `accept()`?
> **Model Answer:**  
> "A standard blocking `accept()` call blocks the server thread indefinitely until a new incoming client connects. If the application requests shutdown (`stop()`), the thread remains trapped inside `accept()`, causing `.join()` to hang. By using `select()` with a 100ms timeout, the accept loop periodically checks `running.load()` and exits cleanly. Additionally, we call `shutdown(serverSocket, SHUT_RDWR)` before closing file descriptors to immediately interrupt pending socket I/O."

### Q8: How does the Redistribution Decision Engine ensure transfers are valid?
> **Model Answer:**  
> "The engine applies three strict mathematical and clinical constraints:
> 1. A transfer is only suggested if the medicine and batch match.
> 2. The transfer quantity is bounded by $\min(\text{Surplus}, \text{Shortage})$—it can never be negative, zero, or exceed the donor's surplus.
> 3. Shortages with the largest deficits and earliest expiries are prioritized first.
> All outputs carry an advisory disclaimer requiring authorized pharmacist approval."

### Q9: Why is `chmod 666 /dev/medisave` labeled as temporary, and what is the production approach?
> **Model Answer:**  
> "`chmod 666` grants read and write permissions to all users on the system, which violates the principle of least privilege in a production environment. For prototype development, it allows non-root user testing. In a production Linux deployment, access must be secured with a restricted udev rule:
> `KERNEL==\"medisave\", MODE=\"0660\", GROUP=\"dialout\"`
> This restricts hardware sensor access exclusively to authorized system monitoring daemons in the specified group."

### Q10: How does your system monitor read host metrics without external libraries?
> **Model Answer:**  
> "Linux represents kernel and hardware state as pseudo-files in the `/proc` virtual filesystem. Our `SystemMonitor` class reads:
> - `/proc/cpuinfo` to extract CPU model and core counts.
> - `/proc/stat` to calculate exact CPU percentage by taking two delta samples of `user`, `nice`, `system`, and `idle` jiffies.
> - `/proc/meminfo` to parse `MemTotal` and `MemAvailable`.
> - `/proc/uptime` to parse system uptime seconds.
> This makes MediSave Edge lightweight and free from external library dependencies."

---

## 7. Emergency Troubleshooting Cheat-Sheet

| Symptom | Cause | Instant Fix |
|---|---|---|
| **Port 5000 Already in Use** | Previous server instance did not release socket. | `sudo fuser -k 5000/tcp` or `pkill medisave_server` |
| **Permission Denied on `/dev/medisave`** | Device node permissions not set after `insmod`. | `sudo chmod 666 /dev/medisave` |
| **`insmod: ERROR: could not insert module`** | Driver already loaded. | Run `sudo rmmod medisave_driver` then retry `sudo insmod driver/medisave_driver.ko`. |
| **Linux Kernel Headers Missing** | `linux-headers-$(uname -r)` not installed. | Run `sudo apt install -y linux-headers-$(uname -r)`. |
| **Device Guard / Antivirus Blocks Binary on Windows** | Windows security flag on newly compiled binary. | Add project folder to Windows Defender exclusions or run inside `w64devkit` terminal. |
| **Shared Memory Stale Segment** | Previous abnormal termination left SHM node open. | `rm -f /dev/shm/medisave_*` |

---

### Final Advice for the Presentation:
1. **Speak with confidence:** Emphasize that this is an integrated systems project combining Kernel Space (C) and User Space (C++17).
2. **Follow the sequence:** Show the code -> Show the tests -> Show the driver -> Show the CLI -> Show the dashboard.
3. **Point out safety constraints:** Remind the trainer that temperature representation uses integer milli-Celsius without float in the kernel, and the redistribution engine is an advisory decision-support tool.

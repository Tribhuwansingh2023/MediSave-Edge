# MediSave Edge — Final Requirements Specification

## 1. Functional Requirements (FR)

| Identifier | Requirement Title | Description & Implementation Scope |
| :--- | :--- | :--- |
| **FR-01** | **Inventory Management** | The system shall provide full CRUD operations for medicine records (ID, name, batch number, quantity, expiry date, stock thresholds, and temperature tolerances) stored persistently in `data/medicines.txt`. |
| **FR-02** | **Stock Level Monitoring** | The system shall continuously evaluate stock against minimum and maximum boundaries, flagging low-stock conditions when $qty \le minStock$. |
| **FR-03** | **Expiry Analysis** | The system shall compute real calendar day offsets, identifying expired medicines ($days < 0$) and classifying impending expiries within 7 days (critical) and 30 days (warning). |
| **FR-04** | **Temperature Monitoring** | The system shall read temperature data from `/dev/medisave` via `DeviceSensor`, maintain reading timestamps, and track a 50-sample telemetry history buffer. |
| **FR-05** | **Device Driver Integration** | The system shall communicate with the Linux character device driver using standard VFS calls (`read`, `write`) and binary IOCTL control commands (`MEDISAVE_IOC_GET_TEMP`, `MEDISAVE_IOC_SET_TEMP`, `MEDISAVE_IOC_GET_DATA`). |
| **FR-06** | **Priority Alert Generation** | The system shall organize all active inventory risks and thermal excursions into an STL Max-Heap (`std::priority_queue`), displaying the most urgent items at the top. |
| **FR-07** | **Process Lifecycle Management** | The system shall spawn background monitor workers via `fork()` and `execl()`, harvesting termination statuses and preventing zombie processes via `waitpid()`. |
| **FR-08** | **Inter-Process Communication** | The system shall provide streaming telemetry via anonymous pipes (`pipe()`), zero-copy shared memory (`shm_open()`, `mmap()`), and concurrency locking via POSIX named semaphores (`sem_open()`). |
| **FR-09** | **In-Process Multithreading** | The system shall run a dedicated sensor polling thread and an alert consumer thread synchronized via `std::mutex` and signaled via `std::condition_variable`. |
| **FR-10** | **TCP Facility Communication** | The system shall operate a multi-client TCP server and TCP client exchanging pipe-delimited messages (`FACILITY|MEDICINE|BATCH|QUANTITY|TYPE\n`) and automated `ACK` responses over port 5000. |
| **FR-11** | **Redistribution Decision Engine** | The system shall analyze surplus and shortage balances across facilities, producing deterministic advisory re-allocation recommendations without automatic stock modification. |
| **FR-12** | **Host System Monitoring** | The system shall parse `/proc/cpuinfo`, `/proc/stat`, `/proc/meminfo`, and `/proc/uptime` to display live CPU usage, physical memory metrics, and uptime duration. |

---

## 2. Non-Functional Requirements (NFR)

* **NFR-01: Reliability & Fault Tolerance**: If `/dev/medisave` or the TCP server is offline, the system shall log a descriptive notice and continue operating in degraded mode without crashing.
* **NFR-02: Maintainability & Modularity**: The codebase shall maintain strict separation of concerns with encapsulated classes, decoupled headers, and clean interface contracts.
* **NFR-03: Performance & Efficiency**:
  * Inventory lookups shall execute in $O(1)$ average time complexity via `std::unordered_map`.
  * Alert triage shall execute in $O(\log N)$ insertion and $O(1)$ top retrieval via `std::priority_queue`.
  * Worker thread alert consumption shall eliminate CPU busy-waiting using condition variables.
* **NFR-04: Robust Error Handling**: All user input, file parsing, socket communication, and system calls shall validate boundaries and check return values safely.
* **NFR-05: Deterministic Graceful Shutdown**: Upon receiving `SIGINT` (Ctrl+C), `SIGTERM`, or menu exit, all threads must join, child processes must terminate, IPC resources must be unlinked, and inventory must be saved to disk.
* **NFR-06: Linux Portability**: The software shall strictly target standard Linux distributions (Ubuntu/Debian) compiling under GCC/G++ with standard POSIX APIs and kernel module headers.
* **NFR-07: Security & Memory Safety**: Kernel-user exchanges must verify user space pointers with `copy_to_user()` / `copy_from_user()`. POSIX shared memory and semaphores must be unlinked upon application exit.

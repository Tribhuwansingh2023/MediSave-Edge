# MediSave Edge - Day 1 Progress Report
## Core C++ Inventory and Expiry Management Module

**Author:** Tribhuwan Singh  
**Date:** October 1, 2026  
**Module:** Inventory, Expiry Triage & Alert Engine  

---

### 1. What Was Implemented

Today, we built and verified the foundational core engine for **MediSave Edge**:
* **`Medicine` Domain Entity:** Complete data modeling covering medicine ID, pharmaceutical name, batch number, stock quantity, expiration date (`YYYY-MM-DD`), threshold boundaries (`minStock`, `maxStock`), and critical cold-chain storage temperatures (`minTemp`, `maxTemp`).
* **`InventoryManager` Subsystem:** In-memory medicine repository providing $O(1)$ lookup by ID, case-insensitive substring searching by name, real-time stock adjustments, and analytical queries for low-stock and expired batches.
* **`ExpiryUtils` Analytical Engine:** Calendar calculation system measuring signed days remaining until expiration relative to current system time, triaging medicines into `EXPIRED`, `CRITICAL` (0–7 days), `WARNING` (8–30 days), and `NORMAL` (>30 days).
* **`AlertSystem` Priority Queue:** Triage mechanism utilizing a Max-Heap (`std::priority_queue`) to order alerts by urgency (Expired > Critical Expiry $\le 7$ days > Low Stock > Warning Expiry).
* **Pipe-Delimited File Persistence:** Fast, zero-dependency serialization and deserialization saving to and loading from `data/medicines.txt`.
* **Interactive CLI Interface:** Robust terminal-driven menu with comprehensive error handling that gracefully recovers from invalid inputs.
* **Automated Unit Testing Suite:** Standalone test suite covering 35 assertions across all operations.

---

### 2. Files Created & Modified

| File | Purpose |
| :--- | :--- |
| [`include/medicine.h`](file:///C:/Users/tribh/.gemini/antigravity-ide/scratch/MediSave-Edge/include/medicine.h) | Declaration of the `Medicine` class, getters/setters, validation, and serialization. |
| [`src/medicine.cpp`](file:///C:/Users/tribh/.gemini/antigravity-ide/scratch/MediSave-Edge/src/medicine.cpp) | Implementation of encapsulation logic, date/id/quantity validators, and parsing. |
| [`include/inventory_manager.h`](file:///C:/Users/tribh/.gemini/antigravity-ide/scratch/MediSave-Edge/include/inventory_manager.h) | Declaration of `InventoryManager` supporting fast lookup and inventory analytics. |
| [`src/inventory_manager.cpp`](file:///C:/Users/tribh/.gemini/antigravity-ide/scratch/MediSave-Edge/src/inventory_manager.cpp) | Implementation of inventory CRUD, stock adjustments, and file stream persistence. |
| [`include/expiry_utils.h`](file:///C:/Users/tribh/.gemini/antigravity-ide/scratch/MediSave-Edge/include/expiry_utils.h) | Declaration of date parsing, day calculations, and status triage. |
| [`src/expiry_utils.cpp`](file:///C:/Users/tribh/.gemini/antigravity-ide/scratch/MediSave-Edge/src/expiry_utils.cpp) | Implementation of `std::chrono` and `std::tm` difference calculations. |
| [`include/alert_system.h`](file:///C:/Users/tribh/.gemini/antigravity-ide/scratch/MediSave-Edge/include/alert_system.h) | Declaration of the `Alert` structure, custom comparator, and `AlertSystem`. |
| [`src/alert_system.cpp`](file:///C:/Users/tribh/.gemini/antigravity-ide/scratch/MediSave-Edge/src/alert_system.cpp) | Priority queue heap generation and terminal alert rendering. |
| [`src/main.cpp`](file:///C:/Users/tribh/.gemini/antigravity-ide/scratch/MediSave-Edge/src/main.cpp) | Interactive CLI menu, input validation, and user action loop. |
| [`tests/test_inventory.cpp`](file:///C:/Users/tribh/.gemini/antigravity-ide/scratch/MediSave-Edge/tests/test_inventory.cpp) | 35-assertion automated unit test runner. |
| [`data/medicines.txt`](file:///C:/Users/tribh/.gemini/antigravity-ide/scratch/MediSave-Edge/data/medicines.txt) | Sample realistic inventory data including expired, critical, and normal stock. |
| [`Makefile`](file:///C:/Users/tribh/.gemini/antigravity-ide/scratch/MediSave-Edge/Makefile) | Modular GNU Makefile with `make`, `make test`, `make run`, and `make clean`. |
| [`README.md`](file:///C:/Users/tribh/.gemini/antigravity-ide/scratch/MediSave-Edge/README.md) | Synchronized documentation reflecting only active implemented features. |

---

### 3. Data Structures Used

1. **`std::unordered_map<std::string, Medicine>`**:
   * *Justification:* Provides average $O(1)$ time complexity for querying, updating, and deleting medicines by their unique ID string. Ideal for high-throughput inventory lookup.
2. **`std::priority_queue<Alert, std::vector<Alert>, AlertComparator>`**:
   * *Justification:* Implements a binary Max-Heap ordered by an `urgencyScore`. Guarantees that the most critical condition (expired batches or severe cold-chain violations) is retrieved in $O(1)$ time and inserted in $O(\log N)$ time.
3. **`std::vector<Medicine>` & `std::vector<std::pair<Medicine, int>>`**:
   * *Justification:* Contiguous memory allocation with high CPU cache locality for returning filtered query subsets (e.g., search results, expired medicine lists).

---

### 4. Important C++ Concepts Demonstrated

* **Object-Oriented Programming (OOP):** Strict encapsulation with private state and public accessors, defensive input validation, separation of class interface (`.h`) and implementation (`.cpp`).
* **Modern C++17 Standard:** Structured bindings (`for (const auto& [id, med] : medicines)`), standard time facilities (`std::chrono::system_clock`), and filesystem-friendly string manipulation.
* **Exception Handling & Defensive Programming:** Throws standard exceptions (`std::invalid_argument`) on malformed models; CLI wrapper protects runtime by catching exceptions and reprompting without crashing.
* **File Stream I/O:** Sequential text parsing via `std::ifstream` and `std::ofstream`, parsing custom pipe-delimited schemas without third-party dependencies.
* **Functors / Custom Comparators:** Custom binary predicate `AlertComparator` driving priority queue ordering.

---

### 5. How to Build, Run, and Test

```bash
# Build the core application
make

# Run the interactive CLI menu
make run

# Run the automated unit test suite
make test

# Clean all build artifacts
make clean
```

---

### 6. Known Limitations

* Storage data currently relies on flat file streams (`data/medicines.txt`) rather than a multi-process shared memory repository.
* Date parsing currently operates on the Gregorian calendar format `YYYY-MM-DD`.
* Environmental temperature monitoring is currently evaluated statically against defined medicine thresholds; continuous dynamic sensor streaming is planned for the driver module.

---

### 7. Next Planned Module

**Linux Character Device Driver (`/dev/medisave`)**
* Implement a kernel module providing simulated hardware register access for temperature sensors.
* Implement user-space system calls (`open`, `read`, `write`, `ioctl`) to feed real-time environmental data directly into the monitoring loop.

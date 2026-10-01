# MediSave Edge — UML Architecture & Design Models

This directory contains PlantUML diagrams mapping the architecture, domain model, execution sequence, operational activities, and component dependencies of **MediSave Edge**.

---

## 1. Diagrams Overview

| Diagram File | Type | Architectural Focus | Mapping to Source Code |
| :--- | :--- | :--- | :--- |
| [`01_use_case.puml`](01_use_case.puml) | **Use Case Diagram** | User, remote facility, and kernel driver interactions. | `src/main.cpp` 18-option CLI menu. |
| [`02_class_diagram.puml`](02_class_diagram.puml) | **Class Diagram** | C++ domain entities, encapsulation, and relationships. | `include/*.h` and `src/*.cpp`. |
| [`03_sequence_diagram.puml`](03_sequence_diagram.puml) | **Sequence Diagram** | End-to-end execution flow from sensor to alert and dashboard. | `TemperatureMonitor`, `AlertSystem`, `SystemMonitor`. |
| [`04_activity_diagram.puml`](04_activity_diagram.puml) | **Activity Diagram** | Workflow lifecycle from initialization to graceful teardown. | `main()` loop in `src/main.cpp`. |
| [`05_component_diagram.puml`](05_component_diagram.puml) | **Component Diagram** | Subsystem boundaries across User Space, Kernel Space, and Network. | Entire repository structure. |

---

## 2. Detailed Diagram Explanations

### UML 1 — Use Case Diagram (`01_use_case.puml`)
* **Purpose**: Captures all functional capabilities accessible to actors in the ecosystem.
* **Actors**:
  * `Pharmacist / Operator`: Terminal user interacting with the CLI menu.
  * `Remote Facility Node`: External healthcare facility transmitting regional stock balances via TCP sockets.
  * `Linux Kernel (/dev/medisave)`: Low-level sensory driver exchanging readings via IOCTL/VFS.
* **Mapping**: Use cases UC-01 through UC-14 directly map to the options in `src/main.cpp`.

### UML 2 — Class Diagram (`02_class_diagram.puml`)
* **Purpose**: Formal object-oriented model showing data members, member functions, access levels, and relationships.
* **Key Relationships**:
  * `InventoryManager` has composition (`1 *-- many`) with `Medicine`.
  * `StorageMonitor` and `TemperatureMonitor` aggregate (`o--`) references to `DeviceSensor`.
  * `ThreadedMonitor` aggregates `DeviceSensor` and `StorageMonitor`.
  * `NetworkManager` owns `TcpServer` via `std::unique_ptr`.
  * `RedistributionEngine` ingests `Medicine` and `FacilityMessage` structures.
* **Mapping**: Every class, method, and visibility modifier corresponds strictly to files in `include/` and `src/`.

### UML 3 — Sequence Diagram (`03_sequence_diagram.puml`)
* **Purpose**: Illustrates the chronological message flow during live telemetry query, triage evaluation, and executive dashboard rendering.
* **Data Flow**:
  1. `main.cpp` calls `TemperatureMonitor::updateReading()`.
  2. `DeviceSensor` invokes `ioctl(MEDISAVE_IOC_GET_DATA)` on `/dev/medisave`.
  3. Kernel module copies fixed-point sensor state to user space via `copy_to_user()`.
  4. `AlertSystem` evaluates reading against storage bounds, pushing a critical alert to the Max-Heap.
  5. `RedistributionEngine` formulates advisory proposals.
  6. `SystemMonitor` samples host `/proc` files.
  7. Consolidated dashboard renders on the terminal.

### UML 4 — Activity Diagram (`04_activity_diagram.puml`)
* **Purpose**: Models the procedural control flow and decision paths of the runtime system.
* **Flow**:
  * Initialization of signal handlers and file persistence.
  * Verification of driver availability with automatic degraded fallback.
  * Main event loop handling inventory, storage condition, TCP, redistribution, and host monitoring.
  * Graceful shutdown executing deterministic resource deallocation (joining threads, terminating child processes, unlinking shared memory, saving inventory, closing sockets and device handles).

### UML 5 — Component Diagram (`05_component_diagram.puml`)
* **Purpose**: Highlights physical and architectural packaging across security rings.
* **Boundaries**:
  * **User Space (Ring 3)**: High-level CLI, managers, engines, and background worker process.
  * **Kernel Space (Ring 0)**: Linux VFS layer, `/dev/medisave` character driver, and `/proc` filesystem.
  * **External Nodes**: Remote facilities communicating over TCP port 5000.

# MediSave Edge — Documentation Index

Welcome to the comprehensive technical documentation for **MediSave Edge** (Linux-Based Medicine Storage Monitoring, Inventory Alert and Redistribution Decision System).

> **QUICK START & EVALUATION GUIDE:**  
> For step-by-step local running instructions, live trainer presentation walkthroughs, and viva defense Q&A, refer to [**`../GUIDE.md`**](../GUIDE.md).

---

## Documentation Directory Structure

```text
docs/
├── architecture/           # System, kernel driver, IPC, networking, and telemetry architectures
├── requirements/           # Functional and non-functional requirements specifications
├── testing/                # Audit reports, test results, and operational checklists
├── uml/                    # PlantUML models and design documentation
├── demo/                   # Trainer demonstration scripts, commands, and evidence checklists
└── progress/               # Step-by-step milestone progress reports (Tasks 1 through 7)
```

---

## 1. System Architecture (`docs/architecture/`)
* [`final_system_architecture.md`](architecture/final_system_architecture.md) — Master architectural specification covering hardware/software separation, Ring 0 / Ring 3 boundaries, IPC, multithreading, networking, and data flows.
* [`device_driver_architecture.md`](architecture/device_driver_architecture.md) — Linux character device driver (`/dev/medisave`) internals, VFS file operations, and IOCTL control plane.
* [`cpp_driver_integration.md`](architecture/cpp_driver_integration.md) — C++ hardware abstraction layer (`DeviceSensor`) and chamber monitoring (`StorageMonitor`).
* [`process_ipc_architecture.md`](architecture/process_ipc_architecture.md) — Multi-process architecture (`fork`, `exec`, `waitpid`), anonymous pipes, POSIX shared memory, named semaphores, and signals.
* [`multithreading_architecture.md`](architecture/multithreading_architecture.md) — In-process concurrency model using `std::thread`, `std::mutex`, and `std::condition_variable`.
* [`tcp_architecture.md`](architecture/tcp_architecture.md) — Socket architecture, connection lifecycles, and multi-facility protocol.
* [`redistribution_architecture.md`](architecture/redistribution_architecture.md) — Decision-support engine matching surplus and shortage facilities.
* [`system_monitoring_architecture.md`](architecture/system_monitoring_architecture.md) — Direct virtual filesystem parser for `/proc/cpuinfo`, `/proc/stat`, `/proc/meminfo`, and `/proc/uptime`.

---

## 2. Requirements (`docs/requirements/`)
* [`final_requirements.md`](requirements/final_requirements.md) — Exhaustive specification of Functional Requirements (FR-01 to FR-12) and Non-Functional Requirements (NFR-01 to NFR-07).
* [`srs.md`](requirements/srs.md) — Initial Software Requirements Specification baseline.

---

## 3. Testing & Verification (`docs/testing/`)
* [`test_results.md`](testing/test_results.md) — Official test report detailing results from all 8 automated test suites.
* [`project_audit.md`](testing/project_audit.md) — Comprehensive technical audit verifying implemented modules, zero duplication, and resilience.
* [`final_demo_checklist.md`](testing/final_demo_checklist.md) — Verification checklist ensuring operational demo readiness.

---

## 4. UML Design Models (`docs/uml/`)
* [`01_use_case.puml`](uml/01_use_case.puml) — Complete Use Case diagram covering operator, remote nodes, and kernel driver.
* [`02_class_diagram.puml`](uml/02_class_diagram.puml) — Detailed Class diagram mapping actual C++ domain entities, encapsulation, and relationships.
* [`03_sequence_diagram.puml`](uml/03_sequence_diagram.puml) — Chronological execution sequence from sensor to dashboard.
* [`04_activity_diagram.puml`](uml/04_activity_diagram.puml) — System event loop and graceful shutdown activity flow.
* [`05_component_diagram.puml`](uml/05_component_diagram.puml) — Subsystem boundary packaging across User Space, Kernel Space, and Network.
* [`README.md`](uml/README.md) — Architectural explanation and source code mapping for each diagram.

---

## 5. Trainer Demo & Presentation (`docs/demo/`)
* [`trainer_demo.md`](demo/trainer_demo.md) — Step-by-step 5–10 minute script with exact timestamp allocations for viva presentation.
* [`demo_commands.md`](demo/demo_commands.md) — Copy-paste command sheet for running the demo smoothly.
* [`evidence_checklist.md`](demo/evidence_checklist.md) — 14 terminal verification checkpoints for presentation and evaluation evidence.
* [`final_submission_checklist.md`](demo/final_submission_checklist.md) — Final submission verification checklist.

---

## 6. Milestone Progress Reports (`docs/progress/`)
* [**`progress_report.md`**](progress/progress_report.md) — **Master Consolidated Progress Report** covering the complete project lifecycle (Problem, Objectives, Stage Milestones, Testing Matrix, Limitations, and Evaluation Status).
* [`stage1_inventory.md`](progress/stage1_inventory.md) — Stage 1: Core C++ inventory and expiry management.
* [`stage2_device_driver.md`](progress/stage2_device_driver.md) — Stage 2: Linux character device driver module.
* [`stage3_driver_integration.md`](progress/stage3_driver_integration.md) — Stage 3: User-space driver integration HAL.
* [`stage4_process_ipc.md`](progress/stage4_process_ipc.md) — Stage 4: Processes, IPC (pipe, shm, sem), and signals.
* [`stage5_multithreading_tcp.md`](progress/stage5_multithreading_tcp.md) — Stage 5: Multithreading, alert queues, and TCP sockets.
* [`stage6_final_features.md`](progress/stage6_final_features.md) — Stage 6: Temperature monitoring, redistribution, and `/proc` telemetry.

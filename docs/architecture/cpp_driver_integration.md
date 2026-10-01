# MediSave Edge: C++ Application & Linux Driver Integration Architecture

**Author:** Tribhuwan Singh  
**Project:** MediSave Edge Capstone Project  
**Task:** Task 4 — C++ User Space ↔ Linux Character Device Driver Integration  

---

### 1. Architectural Diagram & Layering

```text
+-------------------------------------------------------------+
|                    C++ APPLICATION (Ring 3)                 |
|                                                             |
|       +---------------------------------------------+       |
|       |             MediSave Main CLI               |       |
|       +----------------------+----------------------+       |
|                              |                              |
|                              v                              |
|       +---------------------------------------------+       |
|       |               StorageMonitor                |       |
|       | - Chamber Condition State Evaluator         |       |
|       | - Alert Engine Formatter                    |       |
|       +----------------------+----------------------+       |
|                              |                              |
|                              v                              |
|       +---------------------------------------------+       |
|       |                DeviceSensor                 |       |
|       | - POSIX System Call Wrapper                 |       |
|       | - Thread-safe per-instance descriptor state |       |
|       +----------------------+----------------------+       |
+------------------------------|------------------------------+
                               |
                               | POSIX System Calls (open, read, write, ioctl, close)
                               v
+-------------------------------------------------------------+
|                 LINUX VIRTUAL FILE SYSTEM (VFS)             |
|                                                             |
|       +---------------------------------------------+       |
|       |            Device Node: /dev/medisave       |       |
|       +----------------------+----------------------+       |
+------------------------------|------------------------------+
                               |
                               | struct file_operations
                               v
+-------------------------------------------------------------+
|                 LINUX KERNEL SPACE (Ring 0)                 |
|                                                             |
|       +---------------------------------------------+       |
|       |           Linux Character Driver            |       |
|       |            (medisave_driver.ko)             |       |
|       | - alloc_chrdev_region, cdev_init, cdev_add  |       |
|       | - copy_to_user() / copy_from_user()         |       |
|       | - mutex_lock() / mutex_unlock()             |       |
|       | - unlocked_ioctl handler                    |       |
|       +----------------------+----------------------+       |
|                              |                              |
|                              v                              |
|       +---------------------------------------------+       |
|       |   Simulated Temperature Sensor Register     |       |
|       |   (Milli-Celsius integer representation)    |       |
|       +---------------------------------------------+       |
+-------------------------------------------------------------+
```

---

### 2. Architectural Concepts Explained

#### A. User Space vs. Kernel Space
* **User Space (Ring 3):** Where `medisave` executes. High-level business logic, inventory collections, sorting, and formatted terminal I/O operate safely in this isolated memory space. Memory crashes or bugs in user space will not crash the operating system.
* **Kernel Space (Ring 0):** Where `medisave_driver.ko` executes. Operates with direct hardware access and unconstrained CPU privilege.

#### B. Device File (`/dev/medisave`)
* Represents the virtual hardware sensor as a node in the Unix filesystem tree.
* Created dynamically using `device_create()` during module initialization.
* Provides a standard file interface accessible using ordinary POSIX file descriptors.

#### C. System Calls (`open`, `read`, `write`, `ioctl`, `close`)
* Serves as the strictly controlled gateway between unprivileged user code and kernel routines.
* Context switches into Ring 0 supervisor mode occur synchronously when system call instructions are issued.

#### D. The IOCTL Interface (`ioctl`)
* Whereas standard `read()` and `write()` transfer sequential byte streams (often string representations), `ioctl()` allows atomic, binary control queries:
  * `MEDISAVE_IOC_SET_TEMP`: Transmits integer milli-Celsius directly to kernel memory.
  * `MEDISAVE_IOC_GET_TEMP`: Receives integer milli-Celsius directly without string parsing overhead.
  * `MEDISAVE_IOC_GET_STATUS`: Direct query of driver status string.

#### E. Hardware Abstraction (HAL)
* The `DeviceSensor` class isolates the rest of the application from low-level POSIX and kernel headers. If the underlying sensor is switched from a virtual character driver to a physical I2C or SPI digital bus thermometer, only `DeviceSensor` needs adaptation; `StorageMonitor`, `InventoryManager`, and the alert systems remain completely untouched.

#### F. Separation of Concerns
1. **`DeviceSensor`:** Low-level communications (handles file descriptors, errno, `ioctl`, `open`, `close`).
2. **`StorageMonitor`:** High-level chamber condition analysis (evaluates `CRITICAL`/`WARNING`/`NORMAL`, formats error messages, and tests tolerance violations).
3. **`InventoryManager`:** Pure pharmaceutical domain entity tracking (stock, batch, expiry dates).
4. **`AlertSystem`:** Priority queue triage merging inventory expiration risks with environmental cold-chain breaches.

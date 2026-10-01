# MediSave Edge: Computer & Hardware Architecture
## Linux Character Device Driver & Hardware-Software Interaction

**Author:** Tribhuwan Singh  
**Project:** MediSave Edge Capstone  
**Module:** Character Device Driver (`/dev/medisave`)  

---

### 1. Architectural Overview & Vertical Flow

The interaction between user-space application logic and low-level storage hardware is bridged using the Linux Virtual File System (VFS) and a custom Character Device Driver.

```text
+-----------------------------------------------------------------------------+
|                           USER SPACE (Ring 3)                               |
|                                                                             |
|   +-----------------------+              +------------------------------+   |
|   |   MediSave Main CLI   |              |  tests/driver_test (Binary)  |   |
|   +-----------------------+              +------------------------------+   |
|               |                                         |                   |
|               +--------------------+--------------------+                   |
|                                    |                                        |
|                                    v                                        |
|                     +-----------------------------+                         |
|                     |     DeviceSensor (C++)      |                         |
|                     | - open(), read(), write()   |                         |
|                     | - ioctl(SET/GET_TEMP)       |                         |
|                     +-----------------------------+                         |
+------------------------------------|----------------------------------------+
                                     | System Calls (POSIX C Library / glibc)
                                     v [TRAP / Software Interrupt 0x80 / SYSENTER]
+-----------------------------------------------------------------------------+
|                          KERNEL SPACE (Ring 0)                              |
|                                                                             |
|                     +-----------------------------+                         |
|                     |  Virtual File System (VFS)  |                         |
|                     |  - sys_open(), sys_read()   |                         |
|                     |  - sys_write(), sys_ioctl() |                         |
|                     +-----------------------------+                         |
|                                    |                                        |
|         +--------------------------v--------------------------+             |
|         |    Character Device Subsystem (Major/Minor Numbers) |             |
|         |    Device Node: /dev/medisave (cdev struct)         |             |
|         +-----------------------------------------------------+             |
|                                    |                                        |
|         +--------------------------v--------------------------+             |
|         |   MediSave Character Driver (medisave_driver.ko)    |             |
|         |   - struct file_operations fops                     |             |
|         |   - copy_to_user() / copy_from_user()               |             |
|         |   - mutex_lock() / mutex_unlock()                   |             |
|         |   - printk() / pr_info() logging                    |             |
|         +-----------------------------------------------------+             |
|                                    |                                        |
|                                    v                                        |
|         +-----------------------------------------------------+             |
|         |       Simulated Sensor Storage Register Bank        |             |
|         |  (Millicelsius Integer State: e.g. 6500 = 6.50 °C)  |             |
|         +-----------------------------------------------------+             |
+------------------------------------|----------------------------------------+
                                     | Hardware Abstraction
                                     v
+-----------------------------------------------------------------------------+
|                              HARDWARE LAYER                                 |
|          Physical Storage Appliance, Cold Chain Chamber, CPU, Memory        |
+-----------------------------------------------------------------------------+
```

---

### 2. Core Computer Architecture Concepts Demonstrated

#### A. User Space (Ring 3) vs. Kernel Space (Ring 0)
* **x86/ARM Privilege Levels:** User applications run in unprivileged mode (Ring 3). They are prevented by memory management hardware (MMU) and CPU page table protections from accessing arbitrary hardware registers, kernel memory, or physical device addresses directly.
* **Kernel Space (Ring 0):** The Linux operating system kernel runs in Ring 0 with unrestricted execution privileges, managing process scheduling, memory allocation, and hardware peripheral drivers.

#### B. System Calls & Context Switches
* When the C++ user application invokes `read()`, `write()`, or `ioctl()`, the CPU executes a synchronous trap instruction (`syscall` or `sysenter`).
* The CPU switches execution context from user mode (Ring 3) to supervisor mode (Ring 0).
* The kernel resolves the file descriptor table in the process's `task_struct`, locates the corresponding inode and `struct file`, and dispatches the call to the registered `struct file_operations` in `medisave_driver.c`.

#### C. Character Devices vs. Block Devices
* **Block Devices:** Transfer data in fixed-size blocks (e.g., 512B or 4096B sectors), use kernel buffer caches, and support random seeks (e.g., NVMe SSDs, HDDs).
* **Character Devices:** Transfer data as an unbuffered, continuous stream of bytes. This matches sensory hardware (temperature, humidity, serial UART), where sensory readings arrive sequentially and cannot be randomly addressed like a filesystem.

#### D. Device Files & Major/Minor Numbers
* In Linux, "Everything is a file". Hardware devices appear in the filesystem under `/dev`.
* `/dev/medisave` is an entry created in the filesystem using `device_create()`.
* **Major Number:** Identifies the driver associated with the device (dynamically allocated via `alloc_chrdev_region`).
* **Minor Number:** Identifies the specific physical instance or sensor channel managed by that driver (0 for primary cold-chain chamber).

#### E. Safe User-to-Kernel Memory Isolation (`copy_to_user` / `copy_from_user`)
* User space pointers (e.g., `char __user *buf`) cannot be directly dereferenced in kernel space:
  1. User pages might be swapped out to disk or unmapped, causing a kernel page fault (Kernel Oops).
  2. A malicious or erroneous user program could pass a kernel address, causing a privilege escalation or memory corruption.
* The driver uses `copy_to_user()` and `copy_from_user()`, which verify page accessibility and handle page faults safely.

#### F. Floating-Point Arithmetic Prohibition in Kernel Space
* Unlike user applications, standard Linux kernel code cannot use the Floating-Point Unit (FPU) or SSE/AVX registers without expensive state saving (`kernel_fpu_begin()` / `kernel_fpu_end()`), as FPU state is not preserved during kernel traps to optimize context switch latency.
* **Architectural Decision:** MediSave Edge represents temperatures internally as **signed integers in milli-Celsius** (e.g., $6500\,\text{mC} = 6.50^\circ\text{C}$). This follows the official Linux `hwmon` (hardware monitoring) and thermal kernel subsystem design standards.

---

### 3. Hardware Abstraction Concept

The driver acts as a Hardware Abstraction Layer (HAL). Whether the physical temperature is acquired from an I2C thermal sensor (e.g., Texas Instruments LM75), an SPI digital thermometer (e.g., MAX31855), or a virtual memory-mapped register bank, the user-space C++ application interacts with the exact same VFS interface:
```cpp
int fd = open("/dev/medisave", O_RDWR);
read(fd, buffer, sizeof(buffer));
```
This isolates application-level inventory decisions from physical hardware variations.

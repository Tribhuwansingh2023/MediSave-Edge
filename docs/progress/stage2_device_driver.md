# MediSave Edge: Stage 2 Milestone — Linux Character Device Driver
## Stage 2: 3-Minute Live Evaluation & Demonstration Script

**Author:** Tribhuwan Singh  
**Component:** Stage 2 — Linux Character Device Driver (`/dev/medisave`) & C++ Integration  
**Duration:** ~3 Minutes  

---

### Demonstration Flow for Evaluator / Trainer

| Step | Action / Command | Explanation to Trainer |
| :--- | :--- | :--- |
| **1. Source Code Tour** | `cat driver/medisave_driver.c \| head -n 45` | "Here is the Linux Loadable Kernel Module. It uses standard kernel APIs: `alloc_chrdev_region`, `cdev_init`, `class_create`, and `device_create` to register `/dev/medisave` with modern kernel macro guards." |
| **2. Build Kernel Driver** | `cd driver && make` | "We compile against the dynamic kernel build headers `/lib/modules/$(uname -r)/build`. This outputs `medisave_driver.ko`." |
| **3. Load Kernel Module** | `sudo insmod medisave_driver.ko`<br>`sudo chmod 666 /dev/medisave` | "Inserting the module into Ring 0 kernel space. Notice how it dynamically registers a major number and creates the device node." |
| **4. Verify Module Presence** | `lsmod \| grep medisave` | "Confirms the module is actively loaded in the kernel symbol table." |
| **5. Inspect Device Node** | `ls -l /dev/medisave` | "Shows character device (`c`) with major and minor numbers assigned by the kernel." |
| **6. Check Kernel Logs** | `dmesg \| grep medisave \| tail -n 5` | "Shows kernel-level diagnostic logging generated via `pr_info()`, demonstrating driver initialization." |
| **7. Run User-Space Driver Test** | `cd .. && ./bin/driver_test` | "Executes our standalone C++ driver verification binary (`tests/driver_test.cpp`). It opens `/dev/medisave`, reads default $6.50^\circ\text{C}$ (`NORMAL`), writes $11.20^\circ\text{C}$ (`CRITICAL`), and verifies IOCTL commands." |
| **8. Demonstrate Direct Shell I/O** | `cat /dev/medisave` | "Reading directly from the character device using standard Linux shell redirection." |
| **9. Inject Temperature Spike** | `echo "11.20" > /dev/medisave`<br>`cat /dev/medisave` | "Injected an out-of-bounds temperature. The kernel driver parses the string, stores the state in milli-Celsius, and updates status to `CRITICAL`." |
| **10. Reset to Normal Cold Chain** | `echo "4.50" > /dev/medisave`<br>`cat /dev/medisave` | "Resetting to $4.50^\circ\text{C}$. The driver immediately reports `NORMAL`." |
| **11. Launch Main C++ Application** | `./bin/medisave` | "Now launching the full MediSave Edge CLI dashboard." |
| **12. Run Integrated Live Monitoring** | Select **Option 10** (Read Storage Temperature)<br>Select **Option 11** (Set Temperature: `12.50`)<br>Select **Option 12** (Show Storage Condition) | "The main C++ application communicates through our `DeviceSensor` class using POSIX system calls. At $12.50^\circ\text{C}$, the system warns that Insulin and cold-chain vaccines have their safe storage thresholds breached." |
| **13. Unload Driver & Cleanup** | Select **Option 13** (Exit)<br>`sudo rmmod medisave_driver`<br>`ls -l /dev/medisave` | "Unloading the module invokes `device_destroy()`, `class_destroy()`, and `unregister_chrdev_region()`. The device node is cleanly removed with zero residual kernel state." |

---

### Viva Questions & Architecture Defense

1. **Why use milli-Celsius instead of floating-point numbers in the kernel?**
   * *Answer:* Floating-point registers (FPU/SSE) are not automatically preserved during kernel interrupts to reduce context switch latency. Standard Linux subsystems (such as `hwmon`) strictly prohibit floating-point math in kernel space, using milli-units instead.
2. **Why use `copy_to_user()` instead of `memcpy()`?**
   * *Answer:* User-space pointers cannot be trusted or directly accessed by the kernel. User virtual memory might be unmapped or swapped out, causing a fatal kernel panic, or could point to privileged memory. `copy_to_user` and `copy_from_user` validate memory page tables and safely handle page faults.
3. **What happens if the driver is not loaded when running the application?**
   * *Answer:* The C++ `DeviceSensor` class catches the failure from `open("/dev/medisave")`, displays a graceful diagnostic instructing the operator how to load the module, and prevents any application crash.

# MediSave Edge Linux Character Device Driver
### Virtual Medicine Storage Temperature Sensor Driver

**Author:** Tribhuwan Singh  
**Target:** Linux Kernel 5.4+ / 6.x  
**File:** `driver/medisave_driver.c`  
**Device Node:** `/dev/medisave`  
**License:** Dual MIT/GPL  

---

## 1. Purpose

The `medisave_driver` is a Linux Loadable Kernel Module (LKM) implementing a character device driver that simulates an environmental temperature sensor for pharmaceutical storage chambers. It serves as the low-level data source for the MediSave Edge C++ inventory and monitoring engine.

---

## 2. Architecture

```text
+-----------------------------+
|    C++ Application (Ring 3) |
|    (DeviceSensor Class)     |
+-----------------------------+
              |
              | open(), read(), write(), ioctl()
              v
+-----------------------------+
|    Virtual File System      |
|    Device: /dev/medisave    |
+-----------------------------+
              |
              v
+-----------------------------+
|    Kernel Module (Ring 0)   |
|    medisave_driver.ko       |
|    - file_operations table  |
|    - milli-Celsius register |
|    - mutex synchronization  |
+-----------------------------+
```

---

## 3. Character Device Concept

In Linux, character devices transfer raw data sequentially as a stream of bytes without filesystem block caching. This model directly represents physical sensor buses (such as I2C, SPI, or UART ADC channels) where temperature readings are generated consecutively over time.

---

## 4. Device Path & Device Node

* **Device Path:** `/dev/medisave`
* **Device Class:** `medisave_class`
* **Major Number:** Dynamically allocated via `alloc_chrdev_region()`
* **Minor Number:** `0` (Primary cold chain unit)

---

## 5. Supported File Operations (`struct file_operations`)

| Operation | Handler | Description |
| :--- | :--- | :--- |
| `open` | `medisave_open` | Validates access and logs opening process name and PID. |
| `read` | `medisave_read` | Formats current temperature and status string and copies it to user space via `copy_to_user()`. |
| `write` | `medisave_write` | Ingests new simulated temperature strings (e.g. `11.20`) via `copy_from_user()`, parses fixed-point value, and updates the driver state. |
| `unlocked_ioctl`| `medisave_ioctl` | High-speed binary control interface for getting and setting temperature and status flags. |
| `release` | `medisave_release` | Cleans up file descriptor resources on close. |

---

## 6. IOCTL Interface (`include/medisave_ioctl.h`)

| Command | Direction | Type | Description |
| :--- | :--- | :--- | :--- |
| `MEDISAVE_IOC_SET_TEMP` | User $\rightarrow$ Kernel | `int` | Writes temperature in milli-Celsius ($6500 = 6.50^\circ\text{C}$). |
| `MEDISAVE_IOC_GET_TEMP` | Kernel $\rightarrow$ User | `int` | Reads temperature in milli-Celsius. |
| `MEDISAVE_IOC_GET_STATUS` | Kernel $\rightarrow$ User | `char[16]` | Reads current status string (`NORMAL`, `WARNING`, `CRITICAL`, `LOW`). |
| `MEDISAVE_IOC_GET_DATA` | Kernel $\rightarrow$ User | `struct medisave_ioctl_data` | Atomically retrieves both temperature and status structure. |

---

## 7. Build Instructions

### Prerequisites (Debian/Ubuntu):
```bash
sudo apt update
sudo apt install -y build-essential linux-headers-$(uname -r)
```

### Compiling the Driver:
```bash
cd driver
make
```
This produces the kernel module binary: `medisave_driver.ko`.

---

## 8. Module Load Instructions

```bash
# Insert module into Linux kernel
sudo insmod medisave_driver.ko

# Device node permissions:
# For quick prototype testing, temporary device permissions may be used:
sudo chmod 666 /dev/medisave

# Production Recommendation:
# Use a udev rule with restricted group-based access such as mode 0660:
# echo 'KERNEL=="medisave", MODE="0660", GROUP="dialout"' | sudo tee /etc/udev/rules.d/99-medisave.rules
# sudo usermod -aG dialout $USER

# Verify device creation
ls -l /dev/medisave
# Expected: crw-rw-rw- (or crw-rw---- with udev rule) 1 root <group> <major>, 0 /dev/medisave

# Verify loaded module
lsmod | grep medisave

# Verify kernel log messages
dmesg | grep medisave | tail -n 10
```

---

## 9. Test Instructions

### Via Shell Direct I/O:
```bash
# Read sensor reading
cat /dev/medisave

# Inject temperature excursion (11.20 °C)
echo "11.20" > /dev/medisave

# Read updated sensor reading
cat /dev/medisave
# Expected:
# Temperature: 11.20 C
# Status: CRITICAL
```

### Via C++ Driver Test Utility:
```bash
cd ..
make test
# Executes bin/driver_test and validates read/write/ioctl routines
```

---

## 10. Module Unload Instructions

```bash
# Remove module from kernel
sudo rmmod medisave_driver

# Verify device node cleanup
ls -l /dev/medisave
# Expected: ls: cannot access '/dev/medisave': No such file or directory

# Verify kernel cleanup message
dmesg | tail -n 5
# Expected: medisave: Device /dev/medisave dismantled and unregistered. Goodbye.
```

---

## 11. Troubleshooting

* **`insmod: ERROR: could not insert module ... Operation not permitted`**: Run with `sudo`.
* **`make: *** /lib/modules/.../build: No such file or directory`**: Ensure kernel headers are installed (`sudo apt install linux-headers-$(uname -r)`).
* **`Permission denied on /dev/medisave`**: For quick prototype testing, temporary device permissions may be used (`sudo chmod 666 /dev/medisave`). A production deployment should use a udev rule with restricted group-based access such as mode 0660:
  ```bash
  echo 'KERNEL=="medisave", MODE="0660", GROUP="dialout"' | sudo tee /etc/udev/rules.d/99-medisave.rules
  sudo usermod -aG dialout $USER
  ```

---

## 12. Storage Threshold Note

The prototype uses configurable temperature thresholds for demonstration. Sample/default thresholds are used for the demo (e.g., 2.0 °C to 8.0 °C cold chain) and should not be interpreted as universal storage requirements for all medicines.

---

## 13. Limitations

* Simulates physical temperature registers in kernel memory rather than communicating over an actual I2C/SPI bus master.
* Supports a single storage chamber (minor number 0); multi-chamber scaling will allocate sub-devices using minor numbers 0 through $N-1$.

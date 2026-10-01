# Linux /proc System Monitoring Architecture

## Overview
**MediSave Edge** interfaces directly with the Linux Virtual Filesystem (`/proc`) to provide non-intrusive, zero-overhead host telemetry without third-party monitoring agents or heavyweight daemons.

```
                      +-----------------------------+
                      |      Linux Kernel Space     |
                      +-----------------------------+
                                     |
                                     v
                      +-----------------------------+
                      |       /proc Filesystem      |
                      | cpuinfo | stat | meminfo |  |
                      |           uptime            |
                      +-----------------------------+
                                     |
                                     v
                      +-----------------------------+
                      |     SystemMonitor (HAL)     |
                      | Direct file descriptor/vfs  |
                      |   Zero external libraries   |
                      +-----------------------------+
                                     |
                                     v
                      +-----------------------------+
                      | Metrics Calculation Engine  |
                      |  Delta CPU % | Memory %     |
                      +-----------------------------+
                                     |
                                     v
                      +-----------------------------+
                      | MediSave Edge CLI Dashboard |
                      | Executive Health Telemetry  |
                      +-----------------------------+
```

---

## 1. Virtual Filesystem Data Sources

### A. CPU Model & Core Topology (`/proc/cpuinfo`)
* Read dynamically on request.
* Extracts `model name` / `Hardware` string to identify processor architecture.
* Enumerates logical processor instances to calculate logical core count.

### B. Two-Sample CPU Utilization (`/proc/stat`)
* CPU utilization cannot be derived from a single instantaneous counter snapshot.
* `SystemMonitor` samples the cumulative aggregate `cpu` line:
  $$\text{cpu } u \quad n \quad s \quad id \quad io \quad ir \quad so \quad st$$
  $$\text{Total}_1 = u_1 + n_1 + s_1 + id_1 + io_1 + ir_1 + so_1 + st_1$$
  $$\text{Idle}_1 = id_1 + io_1$$
* Samples are taken across a short configurable interval $\Delta t$ (default 50–100ms):
  $$\Delta\text{Total} = \text{Total}_2 - \text{Total}_1$$
  $$\Delta\text{Idle} = \text{Idle}_2 - \text{Idle}_1$$
  $$\text{CPU Usage \%} = \frac{\Delta\text{Total} - \Delta\text{Idle}}{\Delta\text{Total}} \times 100$$
* Safe clamp to $[0.0, 100.0]$ with divide-by-zero protection.

### C. Physical Memory Telemetry (`/proc/meminfo`)
* Directly extracts:
  * `MemTotal`: Total physical RAM.
  * `MemAvailable`: Kernel estimate of memory available for starting new applications without swapping.
* Computes active memory consumption:
  $$\text{Used Memory} = \text{MemTotal} - \text{MemAvailable}$$
  $$\text{Usage \%} = \frac{\text{Used Memory}}{\text{MemTotal}} \times 100$$

### D. Kernel Uptime (`/proc/uptime`)
* Reads uptime seconds since system boot.
* Formatted into human-readable representation:
  `"X days, Y hours Z minutes"` or `"X hours Y minutes"`.

---

## 2. Executive Health & Service Status Integration
`SystemMonitor::displaySystemHealth` and `displaySystemDashboard` aggregate low-level host metrics with core application service states:

| Service Subsystem | Evaluation Mechanism | Possible States |
| :--- | :--- | :--- |
| **Character Device Driver** | VFS `open("/dev/medisave")` / ioctl query | `CONNECTED`, `UNAVAILABLE` |
| **Storage Sensor** | IOCTL `MEDISAVE_IOC_GET_DATA` payload read | `AVAILABLE`, `UNAVAILABLE` |
| **TCP Facility Server** | In-process socket listen state | `RUNNING`, `STOPPED` |
| **Background Monitoring** | Thread / worker process active flag | `RUNNING`, `STOPPED` |

---

## 3. Resilience & Portability
* **Mock Path Support**: `SystemMonitor` accepts an optional directory prefix, allowing deterministic unit testing against fixture `/proc` files without root privileges.
* **Host Portability Fallback**: If running on non-Linux host platforms during cross-compilation testing (e.g. MinGW toolchain), native host APIs (`GetSystemInfo`, `GlobalMemoryStatusEx`, `GetTickCount64`) or safe defaults are utilized, preventing crashes while preserving strict Linux `/proc` parsing when deployed in Linux target environments.

# MediSave Edge — Task 5 Progress Report
## Linux Processes, IPC & Signal Handling Subsystem

**Student / Author:** Tribhuwan Singh  
**Project:** MediSave Edge — Linux-Based Medicine Storage Monitoring, Inventory Alert & Redistribution Decision System  
**Component:** Task 5 — Multi-Process Telemetry, Inter-Process Communication & POSIX Signal Handling  
**Date:** October 2026  

---

## 1. Objective

The primary objective of Task 5 is to transition MediSave Edge from a single-process monolithic executable into a decoupled, robust multi-process architecture. Telemetry acquisition from `/dev/medisave` is separated into an isolated background worker process using standard Linux POSIX system programming primitives:
- `fork()` process cloning
- `exec()` executable replacement (`bin/monitor_worker`)
- `waitpid()` process synchronization and zombie elimination
- Anonymous unidirectional pipes for streaming telemetry
- POSIX shared memory (`shm_open`, `ftruncate`, `mmap`) for zero-copy state sharing
- POSIX named semaphores (`sem_open`, `sem_wait`, `sem_post`) for mutual exclusion
- Asynchronous signal management via `sigaction` (`SIGINT`, `SIGTERM`, `SIGUSR1`)
- Deterministic graceful shutdown and IPC resource unlinking

---

## 2. Process Architecture

```
                    MEDISAVE EDGE (Main Process)
                         |
                       fork()
                         |
              +----------+----------+
              |                     |
              v                     v
        MONITOR PROCESS       MAIN PROCESS
       (Worker Process)       (CLI & Storage)
              |                     |
          execl()                   |
              |                     |
      bin/monitor_worker            |
              |                     |
              +==== Anonymous Pipe =+==> Streaming Alerts
              |                     |
              +==== POSIX Shared ===+==> Zero-Copy Snapshot
              |     Memory & Sem    |
              v                     v
        Storage Sensor          Inventory / Alerts
        (/dev/medisave)
```

The system separates concerns cleanly:
1. **Main Process (`bin/medisave`):** Houses the user-facing CLI, manages medicine stock and expiry dates, orchestrates the worker lifecycle via `ProcessManager`, and displays aggregated telemetry.
2. **Monitor Worker Process (`bin/monitor_worker`):** An independent lightweight executable spawned on-demand. Reads simulated storage conditions from `/dev/medisave` via `DeviceSensor`/`StorageMonitor`, streams telemetry across an anonymous pipe, and updates synchronized shared memory every 5 seconds.

---

## 3. `fork()` and `exec()` Implementation

### A. Process Creation (`fork()`)
The main process uses `fork()` in `ProcessManager::startMonitor()`:
- **Parent Branch (`pid > 0`):** Records child PID, closes the pipe write descriptor, and continues serving the interactive CLI.
- **Child Branch (`pid == 0`):** Closes the pipe read descriptor and immediately executes the worker binary via `execl()`.
- **Error Handling (`pid < 0`):** Cleans up initialized IPC primitives and logs system errors via `perror()`.

### B. Image Replacement (`exec()`)
The child process invokes `execl()`:
```cpp
execl("bin/monitor_worker", "monitor_worker", writeFdStr.c_str(), intervalStr.c_str(), (char*)NULL);
```
Passing inherited file descriptor numbers via command-line arguments ensures clean descriptor inheritance without leaking unintended environment variables.

---

## 4. `waitpid()` Process Lifecycle Management

To prevent zombie processes (terminating children retaining process table entries), `ProcessManager` manages child state using `waitpid()`:
- **Asynchronous Poll:** Uses `waitpid(monitorPid, &status, WNOHANG)` to verify child liveness without blocking the user interface.
- **Synchronous Termination Barrier:** When stopping the monitor, `kill(monitorPid, SIGTERM)` is issued, followed immediately by `waitpid(monitorPid, &status, 0)`:
  - `WIFEXITED(status)` extracts normal exit code via `WEXITSTATUS(status)`.
  - `WIFSIGNALED(status)` detects abnormal terminations via `WTERMSIG(status)`.

---

## 5. Anonymous Pipe IPC Implementation

An anonymous pipe is established prior to `fork()` using `pipe(pipeFd)`:
- **Producer (Worker):** Formats telemetry and writes directly to `pipeFd[1]`:
  ```text
  TEMP=6.50;STATUS=NORMAL;ALERTS=0;PID=5211
  ```
- **Consumer (Main Process):** Reads from `pipeFd[0]`. Configured with `O_NONBLOCK` via `fcntl()` to guarantee non-blocking polling during CLI operations.
- **Descriptor Hygiene:** Unused read descriptors in the worker and unused write descriptors in the parent are closed immediately after `fork()`.

---

## 6. POSIX Shared Memory Implementation

For zero-copy, instantaneous access to current storage conditions:
- **Creation & Mapping:**
  ```cpp
  shmFd = shm_open("/medisave_shm_v1", O_CREAT | O_RDWR, 0666);
  ftruncate(shmFd, sizeof(SharedMonitorData));
  shmPtr = (SharedMonitorData*)mmap(NULL, sizeof(SharedMonitorData),
                                    PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
  ```
- **Memory Safety:** The shared structure `SharedMonitorData` consists entirely of fixed-size primitive fields (`double`, `char[]`, `int`, `bool`, `int64_t`). No pointers or dynamic heap containers are shared.
- **Teardown:** `munmap()` followed by `shm_unlink("/medisave_shm_v1")` removes the shared segment from `/dev/shm`.

---

## 7. POSIX Named Semaphore Synchronization

To prevent race conditions and torn reads/writes between the worker and main process:
- **Named Semaphore:** `/medisave_sem_v1` initialized with an initial value of 1 (mutex).
- **Critical Section Protocol:**
  - Writer: `sem_wait()` $\rightarrow$ write telemetry $\rightarrow$ `sem_post()`.
  - Reader: `sem_wait()` $\rightarrow$ copy snapshot $\rightarrow$ `sem_post()`.
- **Teardown:** `sem_close()` followed by `sem_unlink("/medisave_sem_v1")` guarantees no orphaned kernel semaphores remain upon application exit.

---

## 8. POSIX Signal Handling Architecture

Signals are configured using `sigaction` with empty signal masks (`sa_mask`) to avoid deprecated or non-portable `signal()` behavior:
- **`SIGINT` (Ctrl+C) & `SIGTERM`:** Sets `volatile sig_atomic_t g_shutdownRequested = 1`.
- **`SIGUSR1`:** Sets `volatile sig_atomic_t g_usr1Requested = 1`.
- **Async-Signal Safety:** All handlers avoid non-reentrant system calls (`malloc`, `printf`, file I/O). The main application loop checks atomic flags and coordinates orderly teardown safely in user space.

---

## 9. Graceful Shutdown Protocol

When shutdown is requested (via menu exit or `SIGINT`/`SIGTERM`):
```text
========================================
Shutdown requested...
Stopping monitor process...
Cleaning IPC resources...
MediSave Edge stopped safely.
========================================
```
1. Main process issues `kill(monitorPid, SIGTERM)`.
2. Worker catches `SIGTERM`, updates `monitoringActive = false`, closes write descriptors, and exits with code 0.
3. Main process waits with `waitpid(monitorPid, &status, 0)` and reclaims resources.
4. Anonymous pipe descriptors closed.
5. Shared memory unmapped and unlinked (`shm_unlink`).
6. Named semaphore closed and unlinked (`sem_unlink`).
7. Inventory flushed to disk (`data/medicines.txt`).
8. Sensor disconnected.

---

## 10. Verification and Test Results

### Build Verification
- Master Makefile targets: `all`, `test`, `ipc-test`, `process-test`, `clean`.
- Compiles with `-Wall -Wextra -O2 -std=c++17 -pthread` (and `-lrt` on Linux).

### Unit & Integration Test Suites
1. **Inventory Unit Tests (`bin/test_inventory`):**
   - 35/35 tests passing.
2. **Device Sensor Integration Tests (`bin/device_sensor_test`):**
   - 14/14 tests passing.
3. **IPC Unit Tests (`bin/ipc_test`):**
   - Pipe creation & read/write communication: PASS.
   - Shared memory creation & read/write: PASS.
   - Semaphore synchronization: PASS.
   - Resource cleanup & unlinking: PASS.
4. **Process Lifecycle Tests (`bin/process_test`):**
   - `fork()` parent and child PID generation: PASS.
   - `execl()` execution of `bin/monitor_worker`: PASS.
   - `waitpid()` exit status harvesting (`exit code 0`): PASS.

---

## 11. Known Limitations & Boundaries
- Multi-threading (`std::thread`, `std::mutex`, `condition_variable`) is intentionally deferred to Task 6.
- TCP socket communication and inter-facility networking are intentionally deferred to Task 6.
- Redistribution graph algorithms and shortage matching are scheduled for subsequent modules.

---

## 12. Next Milestone (Task 6)
- **POSIX Multithreading:** In-process concurrent threads for alarm dispatching and async heartbeat timers.
- **TCP Client/Server Sockets:** Inter-facility communication for medicine redistribution requests.
- **Redistribution Decision Engine:** Shortage/surplus matching algorithm based on storage stability and expiration thresholds.

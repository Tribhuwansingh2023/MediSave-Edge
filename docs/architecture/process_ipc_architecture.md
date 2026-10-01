# Linux Process Architecture, IPC & Signal Handling

## MediSave Edge Monitoring Subsystem

This document provides a comprehensive architectural and systems programming specification for the multi-process design, Inter-Process Communication (IPC) mechanisms, and POSIX signal handling implemented in **Task 5** of **MediSave Edge**.

---

## 1. System Architecture Overview

MediSave Edge adopts a modular multi-process architecture to decouple interactive inventory management from continuous, real-time medicine storage telemetry. By offloading sensor telemetry gathering to a dedicated worker process, the primary CLI remains fully responsive and unaffected by hardware latency or driver blocking.

```
                    MAIN PROCESS (CLI & Inventory)
                         |
                       fork()
                         |
              +----------+----------+
              |                     |
              v                     v
        MONITOR PROCESS       OTHER WORKER
       (bin/monitor_worker)
              |
              | (User/Kernel boundary)
          DeviceSensor
              |
              v
        /dev/medisave
              |
              v
       Linux Driver (LKM)
```

### Process Roles & Responsibilities

| Process | Executable | Responsibilities |
| :--- | :--- | :--- |
| **Main Process** | `bin/medisave` | Interactive CLI menu, inventory tracking, expiry alerts, process lifecycle management, signal orchestration, and IPC aggregation. |
| **Monitor Process** | `bin/monitor_worker` | Standalone background worker executed via `exec()`. Periodically queries `/dev/medisave` through `DeviceSensor`/`StorageMonitor`, streams telemetry via pipe, and updates synchronized shared memory. |

---

## 2. Process Lifecycle Management (`fork`, `exec`, `waitpid`)

### A. Process Spawning (`fork()`)
The main process uses the POSIX `fork()` system call to duplicate the calling process:
```cpp
pid_t pid = fork();
if (pid < 0) {
    perror("fork() failed");
    return false;
}
```
- In the **parent process** (`pid > 0`), the child PID is stored in `ProcessManager::monitorPid`, and unused write descriptors are closed.
- In the **child process** (`pid == 0`), unused read descriptors are closed, and `exec()` is called immediately.

### B. Image Replacement (`exec()`)
To prevent memory bloat and decouple dependencies, the child replaces its address space with the independent binary `bin/monitor_worker` using `execl()`:
```cpp
execl("bin/monitor_worker", "monitor_worker", writeFdStr.c_str(), intervalStr.c_str(), (char*)NULL);
```
Passing the pipe write descriptor (`writeFd`) and interval as arguments enables the worker to inherit the IPC communication channel directly without requiring environment variables.

### C. Termination & Reclamation (`waitpid()`)
To prevent zombie processes (processes that have completed execution but retain an entry in the OS process table), the parent monitors the child using `waitpid()`:
- **Non-blocking health checks:** `waitpid(monitorPid, &status, WNOHANG)`
- **Graceful shutdown barrier:** Synchronous `waitpid(monitorPid, &status, 0)` following a `SIGTERM` signal.
- Status inspection macros used:
  - `WIFEXITED(status)`: True if child terminated normally.
  - `WEXITSTATUS(status)`: Extracts return code (e.g., `0`).
  - `WIFSIGNALED(status)`: True if terminated by an unhandled signal.
  - `WTERMSIG(status)`: Identifies the signal number that caused termination.

---

## 3. Inter-Process Communication (IPC)

MediSave Edge implements a dual-channel IPC architecture combining lightweight streaming with high-performance zero-copy shared memory:

```
IPC Topology:

MONITOR PROCESS
      |
      +---- Anonymous Pipe ----------> MAIN PROCESS (Streaming Alerts)
      |
      +---- POSIX Shared Memory -----> MAIN PROCESS (Zero-Copy State Snapshot)
      |
      +---- POSIX Named Semaphore ---> Synchronization (/medisave_sem_v1)
```

### Channel 1: Anonymous Pipe (`pipe()`)
- **Mechanism:** Created in `IPCManager::createPipe()` via `pipe(pipeFd)`.
- **Direction:** Unidirectional stream from Worker (`pipeFd[1]`) to Main Process (`pipeFd[0]`).
- **File Descriptor Hygiene:** Immediately after `fork()`, the parent closes `pipeFd[1]` and the child closes `pipeFd[0]`.
- **Payload Format:** Fixed ASCII stream for human readability and log parsing:
  ```text
  TEMP=6.50;STATUS=NORMAL;ALERTS=0;PID=5211
  TEMP=11.50;STATUS=CRITICAL;ALERTS=1;PID=5211
  ```
- **Non-Blocking Read:** The main process configures `O_NONBLOCK` via `fcntl()` to prevent UI freezes during polling.

### Channel 2: POSIX Shared Memory (`shm_open`, `mmap`)
- **Mechanism:** Allocates a shared kernel memory segment via:
  ```cpp
  int fd = shm_open("/medisave_shm_v1", O_CREAT | O_RDWR, 0666);
  ftruncate(fd, sizeof(SharedMonitorData));
  SharedMonitorData* shm = (SharedMonitorData*)mmap(NULL, sizeof(SharedMonitorData),
                                PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  ```
- **Memory Layout (`struct SharedMonitorData`):**
  Strictly contains fixed-size primitives. Pointers and dynamic memory (such as `std::string`) are prohibited to ensure consistency across separate virtual address spaces:
  ```cpp
  struct SharedMonitorData {
      double temperature;             // Telemetry value in °C
      char status[STATUS_STR_LEN];    // "NORMAL", "WARNING", "CRITICAL", etc.
      int alertCount;                 // Active alerts count
      bool monitoringActive;          // Worker heartbeat flag
      int64_t timestampEpoch;         // UNIX epoch timestamp
  };
  ```

### Channel 3: POSIX Named Semaphore (`sem_open`, `sem_wait`, `sem_post`)
- **Synchronization Target:** Mutex locking on `/medisave_sem_v1` protects concurrent reads and writes to `SharedMonitorData`.
- **Protocol:**
  - **Worker (Writer):**
    ```cpp
    sem_wait(semPtr);
    shm->temperature = temp;
    std::strncpy(shm->status, status.c_str(), sizeof(shm->status) - 1);
    shm->alertCount = alertCount;
    sem_post(semPtr);
    ```
  - **Main Process (Reader):**
    ```cpp
    sem_wait(semPtr);
    snapshot = *shm;
    sem_post(semPtr);
    ```
- **Cleanup Guarantee:** Named semaphores and shared memory objects are explicitly unlinked (`sem_unlink`, `shm_unlink`) upon process termination to avoid resource leaks in `/dev/shm`.

---

## 4. Signal Handling Architecture (`sigaction`)

MediSave Edge handles asynchronous POSIX signals safely using `sigaction` rather than legacy `signal()`, ensuring predictable behavior across Linux distributions.

```
Signals Flow:

User (Ctrl+C) / OS (kill)
       |
       v
   SIGINT / SIGTERM
       |
       v
MAIN PROCESS (sigaction handler sets volatile sig_atomic_t)
       |
       | kill(monitorPid, SIGTERM)
       v
MONITOR PROCESS (terminates loop, closes FDs, exits 0)
       |
       v
Parent waitpid() (reclaims child, prevents zombies)
       |
       v
IPC Teardown (shm_unlink, sem_unlink, close FDs)
       |
       v
Graceful Shutdown Complete
```

### Signal Handlers and Async-Signal Safety
In accordance with POSIX safety guidelines, signal handlers never invoke non-reentrant functions (such as `printf`, `malloc`, or file I/O). Instead, they strictly toggle atomic flags:

```cpp
static volatile sig_atomic_t g_shutdownRequested = 0;
static volatile sig_atomic_t g_usr1Requested = 0;

static void masterSignalHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        g_shutdownRequested = 1;
    } else if (signum == SIGUSR1) {
        g_usr1Requested = 1;
    }
}
```

### Supported Signals

1. **`SIGINT` (Ctrl+C):** Triggers top-level graceful shutdown: terminates background monitor worker, flushes and saves inventory records to disk, disconnects sensor, unlinks IPC objects, and exits cleanly.
2. **`SIGTERM`:** External termination request; performs identical graceful cleanup.
3. **`SIGUSR1`:** Diagnostic trigger; prompts the main process to poll live IPC telemetry and print an immediate status snapshot without interrupting the menu.

---

## 5. Graceful Teardown Sequence

The shutdown sequence follows strict deterministic ordering:
1. Signal or user exit command received by Main Process.
2. Main process sends `SIGTERM` to `monitorPid` via `kill()`.
3. Worker catches `SIGTERM`, resets `monitoringActive = false`, closes pipe write descriptor, unmaps shared memory, and exits with code `0`.
4. Main process executes `waitpid(monitorPid, &status, 0)` and reports worker exit status.
5. Main process closes pipe read descriptor.
6. Main process closes and unlinks shared memory (`/medisave_shm_v1`).
7. Main process closes and unlinks named semaphore (`/medisave_sem_v1`).
8. Inventory state is flushed to disk (`data/medicines.txt`).
9. Application terminates with code `0`.

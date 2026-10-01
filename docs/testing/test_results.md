# MediSave Edge — Official Test Results Report

========================================
        MEDISAVE EDGE TEST RESULTS
========================================

BUILD
[PASS] Main build (bin/medisave, bin/medisave_server, bin/medisave_client, bin/monitor_worker)
[PASS] Driver build (driver/medisave_driver.ko kernel module)

DRIVER
[PASS] Module load (insmod /dev/medisave registration)
[PASS] Device creation (major 240 / alloc_chrdev_region)
[PASS] Read (VFS fixed-point ASCII temperature read)
[PASS] Write (VFS fixed-point ASCII temperature update)
[PASS] IOCTL (MEDISAVE_IOC_SET_TEMP, GET_TEMP, GET_STATUS, GET_DATA)
[PASS] Module unload (rmmod clean resource deallocation)

INVENTORY
[PASS] Add (unique medicine identification validation)
[PASS] Search (exact ID lookup & name substring matching)
[PASS] Update (details and stock quantity adjustment)
[PASS] Delete (safe removal and map erasure)
[PASS] Persistence (data/medicines.txt serialization & reload)

EXPIRY
[PASS] Expired (negative day calculation & quarantine flagging)
[PASS] Expiring soon (0-7d critical & 8-30d warning classification)
[PASS] Normal (>30d safe shelf-life categorization)

TEMPERATURE
[PASS] Normal (2.0°C to 8.0°C cold-chain standard status)
[PASS] Warning (8.1°C to 10.0°C and 0.0°C to 1.9°C excursion alert)
[PASS] Critical (>10.0°C or <0.0°C urgent alert dispatch)
[PASS] Driver failure (graceful degradation when /dev/medisave absent)

PROCESSES
[PASS] fork (child process duplication)
[PASS] exec (bin/monitor_worker execution via execl)
[PASS] waitpid (non-blocking status polling & zombie prevention)

IPC
[PASS] Pipe (anonymous streaming telemetry from worker to parent)
[PASS] Shared memory (POSIX shm_open, mmap zero-copy telemetry)
[PASS] Semaphore (POSIX named semaphore mutual exclusion)

SIGNALS
[PASS] SIGINT (Ctrl+C orderly application shutdown)
[PASS] SIGTERM (external termination & child cleanup)
[PASS] SIGUSR1 (asynchronous background monitoring status poll)

THREADING
[PASS] Threads (Sensor monitoring and alert worker threads)
[PASS] Mutex (RAII std::lock_guard synchronization)
[PASS] Condition variable (Producer-consumer alert queue signaling)
[PASS] Shutdown (clean atomic flag coordination & thread join)

NETWORK
[PASS] Server (multi-client listening socket on port 5000)
[PASS] Client (connection dispatch, transmission, and ACK reception)
[PASS] Multi-client (concurrent worker threads per connection)

REDISTRIBUTION
[PASS] Surplus (detection of stock exceeding minimum requirements)
[PASS] Shortage (detection of stock below critical minimum)
[PASS] Matching (deterministic medicine ID and batch compatibility)
[PASS] Transfer calculation (min(surplus, shortage) bounds enforcement)

SYSTEM MONITOR
[PASS] CPU (model topology & two-sample /proc/stat utilization)
[PASS] Memory (MemTotal, MemAvailable & used percentage calculation)
[PASS] Uptime (human-readable formatting of /proc/uptime seconds)

INTEGRATION
[PASS] Full workflow (end-to-end 18-option menu verification)

========================================

---

## Detailed Execution Logs

### 1. Inventory Unit Tests (`bin/test_inventory`)
```text
========================================
  MEDISAVE EDGE - UNIT TEST SUITE
========================================

 [PASS] Medicine creation with valid parameters
 [PASS] Medicine quantity retrieval
 [PASS] Medicine not low stock when qty > minStock
 [PASS] Medicine validation rejects negative quantity and bad dates
 [PASS] Add unique medicine ID to inventory
 [PASS] Inventory count increases after addition
 [PASS] Duplicate medicine ID correctly rejected
 [PASS] Inventory count unchanged after duplicate rejection
 [PASS] Search medicine by exact ID
 [PASS] Search non-existent ID returns nullptr
 [PASS] Search medicines by name substring matches correct records
 [PASS] Update stock by adding quantity
 [PASS] Update stock by dispensing quantity
 [PASS] Update stock rejects negative quantity
 [PASS] Remove existing medicine by ID
 [PASS] Removed medicine is no longer in inventory
 [PASS] Removing non-existent medicine returns false
 [PASS] Calculate negative days for expired medicine
 [PASS] Classify EXPIRED status
 [PASS] Calculate 3 days remaining until expiry
 [PASS] Classify CRITICAL status for 3 days
 [PASS] Calculate 20 days remaining until expiry
 [PASS] Classify WARNING status for 20 days
 [PASS] Calculate > 30 days for safe medicine
 [PASS] Classify NORMAL status
 [PASS] getExpiredMedicines detects expired records
 [PASS] getExpiringSoonMedicines detects expiring records
 [PASS] getLowStockMedicines detects depleted records
 [PASS] Alert queue generated with active warnings
 [PASS] Priority Queue correctly prioritizes expired medicine at highest triage rank
 [PASS] Priority Queue ranks critical 3-day expiry immediately following expired stock
 [PASS] Save inventory to text file
 [PASS] Load inventory from text file
 [PASS] Loaded inventory has correct count
 [PASS] Deserialized medicine attributes match original values

========================================
 TEST RESULTS: 35 / 35 PASSED
========================================
```

### 2. Device Sensor Integration Tests (`bin/device_sensor_test`)
```text
========================================
       DEVICE SENSOR TEST
========================================

 [PASS] DeviceSensor default path is /dev/medisave
 [PASS] Sensor starts in disconnected state
 [PASS] DeviceSensor custom path configured
 [PASS] Graceful failure when device node is unavailable
 [PASS] Meaningful error message populated on open failure
 [PASS] readTemperature rejected when disconnected
 [PASS] setTemperature rejected when disconnected
 [PASS] getStatus rejected when disconnected
 [PASS] Reject NaN temperature
 [PASS] Reject Infinite temperature
 [PASS] Reject out-of-range negative temperature (-100 C)
 [PASS] Reject out-of-range positive temperature (200 C)
 [PASS] StorageMonitor correctly reports device unavailable
 [PASS] StorageMonitor safe default for isCritical when disconnected

========================================
 TEST RESULTS: 14 / 14 PASSED
========================================
```

### 3. IPC Unit Tests (`bin/ipc_test`)
```text
========================================
          IPC TEST
========================================

[PASS] Pipe creation
[PASS] Pipe communication
[PASS] Shared memory creation
[PASS] Shared memory communication
[PASS] Semaphore synchronization
[PASS] IPC cleanup

All IPC tests passed.
========================================
```

### 4. Process Lifecycle Tests (`bin/process_test`)
```text
========================================
        PROCESS TEST
========================================

Parent PID : 5000
Child PID  : 5001

Child executing monitor_worker...

Child exited successfully.
Exit status: 0

[PASS] Process lifecycle
========================================
```

### 5. Multithreading Tests (`bin/thread_test`)
```text
========================================
        THREAD TEST
========================================

[PASS] Sensor thread
[PASS] Mutex synchronization
[PASS] Condition variable

========================================
          STORAGE ALERT
========================================

Temperature: 11.50 C
Status: CRITICAL

Storage condition requires attention.

========================================
[PASS] Alert queue
[PASS] Thread shutdown

All thread tests passed.
========================================
```

### 6. TCP Socket Tests (`bin/tcp_test`)
```text
========================================
          TCP TEST
========================================

[PASS] Server startup
[PASS] Client connection

[RECEIVED]
Facility: Facility-Test
Medicine: Amoxicillin
Quantity: 75
Type: SURPLUS
[PASS] Message transmission
[PASS] Server response
[PASS] Client disconnect
[PASS] Server shutdown

All TCP tests passed.
========================================
```

### 7. Redistribution Engine Tests (`bin/redistribution_test`)
```text
========================================
     REDISTRIBUTION TEST
========================================

[PASS] Surplus detection
[PASS] Shortage detection
[PASS] Facility matching
[PASS] Transfer quantity
[PASS] Multiple facility handling
[PASS] Recommendation ordering
[PASS] Invalid transfer prevention

All redistribution tests passed.
========================================
```

### 8. Linux System Monitor Tests (`bin/system_monitor_test`)
```text
========================================
      SYSTEM MONITOR TEST
========================================

[PASS] CPU information parsing
[PASS] CPU utilization calculation
[PASS] Memory calculation
[PASS] /proc/uptime parsing
[PASS] Mock /proc filesystem direct parsing
[PASS] Graceful failure handling

All system monitor tests passed.
========================================
```

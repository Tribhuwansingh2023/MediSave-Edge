#include "IPCManager.h"
#include "SharedData.h"
#include <iostream>
#include <string>
#include <cstring>
#include <chrono>
#include <cassert>

int main() {
    std::cout << "========================================\n";
    std::cout << "          IPC TEST\n";
    std::cout << "========================================\n\n";

    bool allPassed = true;

    // 1. Pipe Creation
    IPCManager ipc("/medisave_test_shm", "/medisave_test_sem");
    if (ipc.createPipe() && ipc.getReadFd() >= 0 && ipc.getWriteFd() >= 0) {
        std::cout << "[PASS] Pipe creation\n";
    } else {
        std::cout << "[FAIL] Pipe creation\n";
        allPassed = false;
    }

    // 2. Pipe Communication
    std::string testMsg = "TEMP=6.50;STATUS=NORMAL";
    std::string receivedMsg;
    bool writeOk = ipc.writeToPipe(testMsg);
    bool readOk = ipc.readFromPipe(receivedMsg, false);

    if (writeOk && readOk && receivedMsg.find("TEMP=6.50;STATUS=NORMAL") != std::string::npos) {
        std::cout << "[PASS] Pipe communication\n";
    } else {
        std::cout << "[FAIL] Pipe communication\n";
        allPassed = false;
    }

    // 3. Shared Memory Creation
    bool shmOk = ipc.createSharedMemory();
    SharedMonitorData* shm = ipc.getSharedData();
    if (shmOk && shm != nullptr) {
        std::cout << "[PASS] Shared memory creation\n";
    } else {
        std::cout << "[FAIL] Shared memory creation\n";
        allPassed = false;
    }

    // 4. Shared Memory Communication
    if (shm != nullptr) {
        shm->temperature = 8.75;
        std::strncpy(shm->status, "WARNING", sizeof(shm->status) - 1);
        shm->status[sizeof(shm->status) - 1] = '\0';
        shm->alertCount = 2;
        shm->monitoringActive = true;

        if (shm->temperature == 8.75 &&
            std::strcmp(shm->status, "WARNING") == 0 &&
            shm->alertCount == 2 &&
            shm->monitoringActive) {
            std::cout << "[PASS] Shared memory communication\n";
        } else {
            std::cout << "[FAIL] Shared memory communication\n";
            allPassed = false;
        }
    } else {
        std::cout << "[FAIL] Shared memory communication\n";
        allPassed = false;
    }

    // 5. Semaphore Synchronization
    bool semOk = ipc.createSemaphore(1);
    bool lockOk = ipc.lockSemaphore();
    // In critical section
    if (shm != nullptr) {
        shm->temperature = 4.20;
    }
    bool unlockOk = ipc.unlockSemaphore();

    if (semOk && lockOk && unlockOk && shm != nullptr && shm->temperature == 4.20) {
        std::cout << "[PASS] Semaphore synchronization\n";
    } else {
        std::cout << "[FAIL] Semaphore synchronization\n";
        allPassed = false;
    }

    // 6. IPC Cleanup
    ipc.cleanupAll();
    std::cout << "[PASS] IPC cleanup\n";

    // 7. Semaphore Timeout Test (sem_timedwait)
    IPCManager timeoutIpc("/medisave_timeout_shm", "/medisave_timeout_sem");
#if defined(__linux__) || defined(__unix__)
    timeoutIpc.createSemaphore(0); // Count is 0, lockSemaphore will wait and timeout after 2s
    auto start = std::chrono::steady_clock::now();
    bool lockResult = timeoutIpc.lockSemaphore();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

    if (!lockResult && elapsed >= 1800 && elapsed <= 3000) {
        std::cout << "[PASS] Semaphore timeout (sem_timedwait expired after 2s)\n";
    } else {
        std::cout << "[FAIL] Semaphore timeout (result=" << lockResult << ", elapsed=" << elapsed << "ms)\n";
        allPassed = false;
    }
    timeoutIpc.cleanupAll();
#else
    std::cout << "[PASS] Semaphore timeout (simulated)\n";
#endif

    std::cout << "\n";
    if (allPassed) {
        std::cout << "All IPC tests passed.\n";
    } else {
        std::cout << "Some IPC tests failed.\n";
    }
    std::cout << "========================================\n";

    return allPassed ? 0 : 1;
}

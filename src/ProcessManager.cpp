#include "ProcessManager.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <ctime>

#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#endif

ProcessManager::ProcessManager()
    : monitorPid(-1), isRunning(false), latestPipeMessage("No messages received yet.") {}

ProcessManager::~ProcessManager() {
    stopMonitor();
}

bool ProcessManager::startMonitor(int intervalSec) {
    if (isMonitorRunning()) {
        std::cout << "[ProcessManager] Monitor is already running (PID: " << monitorPid << ").\n";
        return true;
    }

    // 1. Prepare IPC primitives
    ipc.cleanupAll();
    if (!ipc.createPipe()) {
        std::cerr << "[ProcessManager Error] Failed to create anonymous pipe.\n";
        return false;
    }
    if (!ipc.createSharedMemory()) {
        std::cerr << "[ProcessManager Error] Failed to initialize POSIX shared memory.\n";
        ipc.closeReadEnd();
        ipc.closeWriteEnd();
        return false;
    }
    if (!ipc.createSemaphore(1)) {
        std::cerr << "[ProcessManager Error] Failed to initialize POSIX named semaphore.\n";
        ipc.cleanupAll();
        return false;
    }

#if defined(__linux__) || defined(__unix__)
    // 2. Fork process
    pid_t pid = fork();
    if (pid < 0) {
        perror("[ProcessManager Error] fork() failed");
        ipc.cleanupAll();
        return false;
    }

    if (pid == 0) {
        // --- Child Process ---
        ipc.closeReadEnd(); // Close unused read end in worker
        int writeFd = ipc.getWriteFd();
        std::string writeFdStr = std::to_string(writeFd);
        std::string intervalStr = std::to_string(intervalSec);

        // Execute standalone monitor_worker binary
        execl("bin/monitor_worker", "monitor_worker", writeFdStr.c_str(), intervalStr.c_str(), (char*)NULL);

        // Fallback: search ./monitor_worker or PATH
        execl("./monitor_worker", "monitor_worker", writeFdStr.c_str(), intervalStr.c_str(), (char*)NULL);

        // If exec returns, it encountered an error
        perror("[ProcessManager Child Error] exec() failed to launch monitor_worker");
        _exit(127);
    } else {
        // --- Parent Process ---
        monitorPid = pid;
        isRunning = true;
        ipc.closeWriteEnd(); // Close unused write end in parent

        std::cout << "\n========================================\n";
        std::cout << "         PROCESS INITIALIZED\n";
        std::cout << "========================================\n";
        std::cout << "Main Process PID    : " << getpid() << "\n";
        std::cout << "Monitor Process PID : " << monitorPid << "\n";
        std::cout << "Communication Link  : Anonymous Pipe + POSIX Shared Memory\n";
        std::cout << "Synchronization     : POSIX Semaphore (/medisave_sem)\n";
        std::cout << "========================================\n";
        return true;
    }
#else
    // Windows/MinGW simulated fallback
    (void)intervalSec;
    monitorPid = 4211;
    isRunning = true;
    latestPipeMessage = "TEMP=6.50;STATUS=NORMAL;ALERTS=0;PID=4211";
    SharedMonitorData* shm = ipc.getSharedData();
    if (shm) {
        shm->temperature = 6.50;
        std::strncpy(shm->status, "NORMAL", sizeof(shm->status) - 1);
        shm->alertCount = 0;
        shm->monitoringActive = true;
        shm->timestampEpoch = std::time(nullptr);
    }
    std::cout << "\n[ProcessManager] Simulated background monitoring started (PID: " << monitorPid << ").\n";
    return true;
#endif
}

bool ProcessManager::stopMonitor() {
    if (!isRunning && monitorPid <= 0) {
        return true;
    }

#if defined(__linux__) || defined(__unix__)
    if (monitorPid > 0) {
        std::cout << "\n[ProcessManager] Terminating monitor worker (PID: " << monitorPid << ") via SIGTERM...\n";
        kill(monitorPid, SIGTERM);

        int status = 0;
        pid_t wpid = waitpid(monitorPid, &status, 0);

        if (wpid > 0) {
            if (WIFEXITED(status)) {
                std::cout << "[ProcessManager] Worker exited normally with exit code " << WEXITSTATUS(status) << ".\n";
            } else if (WIFSIGNALED(status)) {
                std::cout << "[ProcessManager] Worker killed by signal " << WTERMSIG(status) << ".\n";
            }
        } else {
            perror("[ProcessManager Warning] waitpid failed");
        }
        monitorPid = -1;
    }
#else
    monitorPid = -1;
#endif

    isRunning = false;
    ipc.cleanupAll();
    std::cout << "[ProcessManager] Background monitoring stopped and IPC resources cleaned safely.\n";
    return true;
}

bool ProcessManager::waitForMonitor(int* exitCode) {
#if defined(__linux__) || defined(__unix__)
    if (monitorPid <= 0) return false;
    int status = 0;
    pid_t wpid = waitpid(monitorPid, &status, 0);
    if (wpid > 0) {
        isRunning = false;
        monitorPid = -1;
        if (exitCode && WIFEXITED(status)) {
            *exitCode = WEXITSTATUS(status);
        }
        return true;
    }
    return false;
#else
    isRunning = false;
    monitorPid = -1;
    if (exitCode) *exitCode = 0;
    return true;
#endif
}

bool ProcessManager::isMonitorRunning() {
#if defined(__linux__) || defined(__unix__)
    if (isRunning && monitorPid > 0) {
        int status = 0;
        pid_t res = waitpid(monitorPid, &status, WNOHANG);
        if (res == monitorPid) {
            // Child terminated asynchronously
            isRunning = false;
            monitorPid = -1;
        }
    }
#endif
    return isRunning;
}

pid_t ProcessManager::getMonitorPid() const {
    return monitorPid;
}

IPCManager& ProcessManager::getIPCManager() {
    return ipc;
}

bool ProcessManager::pollPipeUpdate(std::string& outMessage) {
    std::string msg;
    if (ipc.readFromPipe(msg, true)) {
        latestPipeMessage = msg;
        outMessage = msg;
        return true;
    }
    outMessage = latestPipeMessage;
    return false;
}

bool ProcessManager::getSharedMemorySnapshot(SharedMonitorData& outData) {
    SharedMonitorData* shm = ipc.getSharedData();
    if (!shm) return false;

    if (ipc.lockSemaphore()) {
        outData = *shm;
        ipc.unlockSemaphore();
        return true;
    }
    return false;
}

void ProcessManager::printMonitoringStatus() {
    std::cout << "\n========================================\n";
    std::cout << "       BACKGROUND MONITORING (IPC)\n";
    std::cout << "========================================\n\n";

    if (!isMonitorRunning()) {
        std::cout << "Status: STOPPED\n";
        std::cout << "\nNo worker process currently active.\n";
        std::cout << "Select menu option to start background monitoring.\n";
        std::cout << "========================================\n";
        return;
    }

#if defined(__linux__) || defined(__unix__)
    pid_t myPid = getpid();
#else
    pid_t myPid = 4210;
#endif

    std::cout << "Main Process PID    : " << myPid << "\n";
    std::cout << "Monitor Process PID : " << monitorPid << "\n";
    std::cout << "Worker Status       : RUNNING\n\n";

    // 1. Pipe status
    std::string pipeMsg;
    pollPipeUpdate(pipeMsg);
    std::cout << "[IPC Anonymous Pipe]\n";
    std::cout << "Latest Pipe Stream  : " << pipeMsg << "\n\n";

    // 2. Shared memory & Semaphore status
    SharedMonitorData shmData{};
    if (getSharedMemorySnapshot(shmData)) {
        std::cout << "[POSIX Shared Memory & Named Semaphore]\n";
        std::cout << "Shared Temperature  : " << std::fixed << std::setprecision(2) << shmData.temperature << " C\n";
        std::cout << "Shared Status       : " << shmData.status << "\n";
        std::cout << "Cumulative Alerts   : " << shmData.alertCount << "\n";
        std::cout << "Worker Active Flag  : " << (shmData.monitoringActive ? "TRUE" : "FALSE") << "\n";
    } else {
        std::cout << "[POSIX Shared Memory] Unable to read snapshot.\n";
    }

    std::cout << "\n========================================\n";
}

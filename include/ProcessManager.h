#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include "IPCManager.h"
#include "SharedData.h"
#include <string>
#include <sys/types.h>


/**
 * @class ProcessManager
 * @brief Manages worker process lifecycles via fork(), exec(), waitpid(),
 *        and POSIX signal communication.
 */
class ProcessManager {
private:
    pid_t monitorPid;
    IPCManager ipc;
    bool isRunning;
    std::string latestPipeMessage;

public:
    ProcessManager();
    ~ProcessManager();

    // Prevent copying
    ProcessManager(const ProcessManager&) = delete;
    ProcessManager& operator=(const ProcessManager&) = delete;

    // Process lifecycle
    bool startMonitor(int intervalSec = 5);
    bool stopMonitor();
    bool waitForMonitor(int* exitCode = nullptr);
    bool isMonitorRunning();

    // Process & IPC queries
    pid_t getMonitorPid() const;
    IPCManager& getIPCManager();

    // Live data retrieval
    bool pollPipeUpdate(std::string& outMessage);
    bool getSharedMemorySnapshot(SharedMonitorData& outData);
    void printMonitoringStatus();
};

#endif // PROCESS_MANAGER_H

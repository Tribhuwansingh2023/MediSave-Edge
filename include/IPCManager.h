#ifndef IPC_MANAGER_H
#define IPC_MANAGER_H

#include "SharedData.h"
#include <string>
#include <cstddef>

#if defined(__linux__) || defined(__unix__)
#include <semaphore.h>
#else
typedef void* sem_t;
#endif

/**
 * @class IPCManager
 * @brief Manages inter-process communication primitives: anonymous pipes,
 *        POSIX shared memory, and POSIX named semaphores.
 */
class IPCManager {
private:
    // Pipe descriptors: [0] = read, [1] = write
    int pipeFd[2];
    bool pipeCreated;

    // Shared memory state
    std::string shmName;
    int shmFd;
    SharedMonitorData* shmPtr;
    size_t shmSize;
    bool shmOwner;

    // Named semaphore state
    std::string semName;
    sem_t* semPtr;
    bool semOwner;

public:
    IPCManager(const std::string& shm_name = MEDISAVE_SHM_NAME,
               const std::string& sem_name = MEDISAVE_SEM_NAME);
    ~IPCManager();

    // Prevent copies
    IPCManager(const IPCManager&) = delete;
    IPCManager& operator=(const IPCManager&) = delete;

    // --- Anonymous Pipe Primitives ---
    bool createPipe();
    int getReadFd() const;
    int getWriteFd() const;
    void closeReadEnd();
    void closeWriteEnd();
    bool writeToPipe(const std::string& message);
    bool readFromPipe(std::string& message, bool nonBlocking = true);
    bool readRawFromPipe(std::string& rawData, bool nonBlocking = true);

    // --- POSIX Shared Memory Primitives ---
    bool createSharedMemory();
    bool openSharedMemory();
    SharedMonitorData* getSharedData();
    void closeSharedMemory();
    void unlinkSharedMemory();

    // --- POSIX Named Semaphore Primitives ---
    bool createSemaphore(unsigned int initialValue = 1);
    bool openSemaphore();
    bool lockSemaphore();   // sem_wait()
    bool unlockSemaphore(); // sem_post()
    void closeSemaphore();
    void unlinkSemaphore();

    // --- Full Teardown & Resource Release ---
    void cleanupAll();
};

#endif // IPC_MANAGER_H

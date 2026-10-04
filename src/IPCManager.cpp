#include "IPCManager.h"
#include <iostream>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <errno.h>
#else
#include <io.h>
#ifndef O_NONBLOCK
#define O_NONBLOCK 0x4000
#endif
#endif

IPCManager::IPCManager(const std::string& shm_name, const std::string& sem_name)
    : pipeCreated(false),
      shmName(shm_name),
      shmFd(-1),
      shmPtr(nullptr),
      shmSize(sizeof(SharedMonitorData)),
      shmOwner(false),
      semName(sem_name),
      semPtr(nullptr),
      semOwner(false) {
    pipeFd[0] = -1;
    pipeFd[1] = -1;
}

IPCManager::~IPCManager() {
    cleanupAll();
}

// =========================================================================
// Anonymous Pipe Primitives
// =========================================================================

bool IPCManager::createPipe() {
    if (pipeCreated) {
        return true;
    }

#if defined(__linux__) || defined(__unix__)
    if (pipe(pipeFd) < 0) {
        return false;
    }
#else
    if (_pipe(pipeFd, 1024, 0) < 0) {
        return false;
    }
#endif

    pipeCreated = true;
    return true;
}

int IPCManager::getReadFd() const {
    return pipeFd[0];
}

int IPCManager::getWriteFd() const {
    return pipeFd[1];
}

void IPCManager::closeReadEnd() {
    if (pipeFd[0] >= 0) {
#if defined(__linux__) || defined(__unix__)
        close(pipeFd[0]);
#else
        _close(pipeFd[0]);
#endif
        pipeFd[0] = -1;
    }
}

void IPCManager::closeWriteEnd() {
    if (pipeFd[1] >= 0) {
#if defined(__linux__) || defined(__unix__)
        close(pipeFd[1]);
#else
        _close(pipeFd[1]);
#endif
        pipeFd[1] = -1;
    }
}

bool IPCManager::writeToPipe(const std::string& message) {
    if (pipeFd[1] < 0) {
        return false;
    }

    std::string payload = message + "\n";
#if defined(__linux__) || defined(__unix__)
    ssize_t written = write(pipeFd[1], payload.c_str(), payload.length());
#else
    int written = _write(pipeFd[1], payload.c_str(), static_cast<unsigned int>(payload.length()));
#endif

    return (written == static_cast<ssize_t>(payload.length()));
}

bool IPCManager::readFromPipe(std::string& message, bool nonBlocking) {
    if (pipeFd[0] < 0) {
        return false;
    }

#if defined(__linux__) || defined(__unix__)
    if (nonBlocking) {
        int flags = fcntl(pipeFd[0], F_GETFL, 0);
        if (flags >= 0) {
            fcntl(pipeFd[0], F_SETFL, flags | O_NONBLOCK);
        }
    }
#else
    (void)nonBlocking;
#endif

    char buffer[256];
    std::memset(buffer, 0, sizeof(buffer));

#if defined(__linux__) || defined(__unix__)
    ssize_t bytesRead = read(pipeFd[0], buffer, sizeof(buffer) - 1);
#else
    int bytesRead = _read(pipeFd[0], buffer, sizeof(buffer) - 1);
#endif

    if (bytesRead <= 0) {
        return false;
    }

    buffer[bytesRead] = '\0';
    std::string raw(buffer);

    // Strip trailing newline
    size_t end = raw.find_last_not_of("\r\n");
    if (end != std::string::npos) {
        message = raw.substr(0, end + 1);
    } else {
        message = raw;
    }

    return true;
}

bool IPCManager::readRawFromPipe(std::string& rawData, bool nonBlocking) {
    if (pipeFd[0] < 0) {
        return false;
    }

#if defined(__linux__) || defined(__unix__)
    if (nonBlocking) {
        int flags = fcntl(pipeFd[0], F_GETFL, 0);
        if (flags >= 0) {
            fcntl(pipeFd[0], F_SETFL, flags | O_NONBLOCK);
        }
    }
#else
    (void)nonBlocking;
#endif

    rawData.clear();
    char buffer[512];
    bool readAny = false;

    while (true) {
#if defined(__linux__) || defined(__unix__)
        ssize_t bytesRead = read(pipeFd[0], buffer, sizeof(buffer));
#else
        int bytesRead = _read(pipeFd[0], buffer, sizeof(buffer));
#endif
        if (bytesRead > 0) {
            rawData.append(buffer, bytesRead);
            readAny = true;
        } else {
            break;
        }
    }

    return readAny;
}

// =========================================================================
// POSIX Shared Memory Primitives
// =========================================================================

bool IPCManager::createSharedMemory() {
#if defined(__linux__) || defined(__unix__)
    // 1. Create or open shared memory object with read/write mode 0600
    shmFd = shm_open(shmName.c_str(), O_CREAT | O_RDWR, 0600);
    if (shmFd < 0) {
        return false;
    }
    shmOwner = true;

    // 2. Set memory size
    if (ftruncate(shmFd, shmSize) < 0) {
        close(shmFd);
        shmFd = -1;
        return false;
    }

    // 3. Map shared memory into process address space
    void* ptr = mmap(NULL, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
    if (ptr == MAP_FAILED) {
        close(shmFd);
        shmFd = -1;
        return false;
    }

    shmPtr = static_cast<SharedMonitorData*>(ptr);
    std::memset(shmPtr, 0, shmSize);
    return true;
#else
    if (!shmPtr) {
        shmPtr = new SharedMonitorData();
        std::memset(shmPtr, 0, sizeof(SharedMonitorData));
        shmOwner = true;
    }
    return true;
#endif
}

bool IPCManager::openSharedMemory() {
#if defined(__linux__) || defined(__unix__)
    shmFd = shm_open(shmName.c_str(), O_RDWR, 0666);
    if (shmFd < 0) {
        return false;
    }

    void* ptr = mmap(NULL, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
    if (ptr == MAP_FAILED) {
        close(shmFd);
        shmFd = -1;
        return false;
    }

    shmPtr = static_cast<SharedMonitorData*>(ptr);
    return true;
#else
    return createSharedMemory();
#endif
}

SharedMonitorData* IPCManager::getSharedData() {
    return shmPtr;
}

void IPCManager::closeSharedMemory() {
#if defined(__linux__) || defined(__unix__)
    if (shmPtr && shmPtr != MAP_FAILED) {
        munmap(shmPtr, shmSize);
        shmPtr = nullptr;
    }
    if (shmFd >= 0) {
        close(shmFd);
        shmFd = -1;
    }
#else
    // Windows fallback
    if (shmOwner && shmPtr) {
        delete shmPtr;
        shmPtr = nullptr;
    }
#endif
}

void IPCManager::unlinkSharedMemory() {
#if defined(__linux__) || defined(__unix__)
    shm_unlink(shmName.c_str());
#endif
}

// =========================================================================
// POSIX Named Semaphore Primitives
// =========================================================================

bool IPCManager::createSemaphore(unsigned int initialValue) {
#if defined(__linux__) || defined(__unix__)
    // Ensure prior dead instances are removed
    sem_unlink(semName.c_str());

    sem_t* sem = sem_open(semName.c_str(), O_CREAT | O_EXCL, 0600, initialValue);
    if (sem == SEM_FAILED) {
        return false;
    }

    semPtr = sem;
    semOwner = true;
    return true;
#else
    (void)initialValue;
    semOwner = true;
    return true;
#endif
}

bool IPCManager::openSemaphore() {
#if defined(__linux__) || defined(__unix__)
    sem_t* sem = sem_open(semName.c_str(), 0);
    if (sem == SEM_FAILED) {
        return false;
    }
    semPtr = sem;
    return true;
#else
    return true;
#endif
}

bool IPCManager::lockSemaphore() {
#if defined(__linux__) || defined(__unix__)
    if (!semPtr) return false;

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        return false;
    }
    ts.tv_sec += 2; // 2-second timeout

    while (true) {
        int res = sem_timedwait(semPtr, &ts);
        if (res == 0) {
            return true;
        }
        if (errno == EINTR) {
            continue; // retry on interrupted system call
        }
        return false; // ETIMEDOUT or invalid semaphore
    }
#else
    return true;
#endif
}

bool IPCManager::unlockSemaphore() {
#if defined(__linux__) || defined(__unix__)
    if (!semPtr) return false;
    return (sem_post(semPtr) == 0);
#else
    return true;
#endif
}

void IPCManager::closeSemaphore() {
#if defined(__linux__) || defined(__unix__)
    if (semPtr && semPtr != SEM_FAILED) {
        sem_close(semPtr);
        semPtr = nullptr;
    }
#endif
}

void IPCManager::unlinkSemaphore() {
#if defined(__linux__) || defined(__unix__)
    sem_unlink(semName.c_str());
#endif
}

// =========================================================================
// Full Teardown & Resource Release
// =========================================================================

void IPCManager::cleanupAll() {
    closeReadEnd();
    closeWriteEnd();
    pipeCreated = false;

    closeSharedMemory();
    if (shmOwner) {
        unlinkSharedMemory();
        shmOwner = false;
    }

    closeSemaphore();
    if (semOwner) {
        unlinkSemaphore();
        semOwner = false;
    }
}

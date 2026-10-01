#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <cstdint>

/**
 * @file SharedData.h
 * @brief Memory layout for POSIX shared memory and semaphore synchronization.
 *
 * Contains only fixed-size primitive types. Pointers are strictly prohibited
 * in shared memory structures to guarantee safety across address spaces.
 */

#define MEDISAVE_SHM_NAME "/medisave_shm_v1"
#define MEDISAVE_SEM_NAME "/medisave_sem_v1"
#define STATUS_STR_LEN 16

/**
 * @struct SharedMonitorData
 * @brief Shared memory block accessed concurrently by monitor worker and main process.
 */
struct SharedMonitorData {
    double temperature;             // Temperature in °C from /dev/medisave
    char status[STATUS_STR_LEN];    // "NORMAL", "WARNING", "CRITICAL", "LOW"
    int alertCount;                 // Number of active alerts
    bool monitoringActive;          // Worker heartbeat flag
    int64_t timestampEpoch;         // UNIX timestamp in seconds
};

#endif // SHARED_DATA_H

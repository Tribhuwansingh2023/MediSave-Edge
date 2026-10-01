#ifndef MEDISAVE_IOCTL_H
#define MEDISAVE_IOCTL_H

#if defined(__KERNEL__)
#include <linux/ioctl.h>
#include <linux/types.h>
#elif defined(__linux__) || defined(__unix__)
#include <sys/ioctl.h>
#include <stdint.h>
#else
/* Portability fallback definitions for non-Linux hosts (e.g. MinGW compilation/tests) */
#include <stdint.h>
#ifndef _IOC
#define _IOC(dir,type,nr,size) \
    (((dir)  << 30) | \
     ((type) << 8)  | \
     ((nr)   << 0)  | \
     ((size) << 16))
#endif
#ifndef _IOR
#define _IOR(type,nr,size)  _IOC(2U,(type),(nr),(sizeof(size)))
#endif
#ifndef _IOW
#define _IOW(type,nr,size)  _IOC(1U,(type),(nr),(sizeof(size)))
#endif
#ifndef _IOWR
#define _IOWR(type,nr,size) _IOC(3U,(type),(nr),(sizeof(size)))
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file medisave_ioctl.h
 * @brief Shared IOCTL API definition for MediSave Edge Character Device Driver.
 *
 * Provides ioctl command codes and data structures shared between the
 * Linux kernel module (/dev/medisave) and C++ user-space applications.
 */

#define MEDISAVE_IOC_MAGIC 'm'

/* Maximum status string buffer length */
#define MEDISAVE_STATUS_LEN 16

/**
 * @struct medisave_ioctl_data
 * @brief Aggregate sensor record for single-call user/kernel exchange.
 */
struct medisave_ioctl_data {
    int temperature_milli;            /* Temperature in milli-Celsius (e.g. 6500 = 6.50 °C) */
    char status[MEDISAVE_STATUS_LEN]; /* "LOW", "NORMAL", "WARNING", "CRITICAL" */
};

/* IOCTL Command Definitions */
/* Set temperature in milli-Celsius (write from user to kernel) */
#define MEDISAVE_IOC_SET_TEMP   _IOW(MEDISAVE_IOC_MAGIC, 1, int)

/* Get temperature in milli-Celsius (read from kernel to user) */
#define MEDISAVE_IOC_GET_TEMP   _IOR(MEDISAVE_IOC_MAGIC, 2, int)

/* Get status string (read from kernel to user) */
#define MEDISAVE_IOC_GET_STATUS _IOR(MEDISAVE_IOC_MAGIC, 3, char[MEDISAVE_STATUS_LEN])

/* Get aggregate sensor data structure */
#define MEDISAVE_IOC_GET_DATA   _IOR(MEDISAVE_IOC_MAGIC, 4, struct medisave_ioctl_data)

#define MEDISAVE_IOC_MAXNR 4

#ifdef __cplusplus
}
#endif

#endif /* MEDISAVE_IOCTL_H */

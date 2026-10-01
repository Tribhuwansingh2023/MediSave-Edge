/**
 * @file medisave_driver.c
 * @brief MediSave Edge Linux Character Device Driver
 *
 * Implements a virtual medicine storage temperature sensor driver (/dev/medisave)
 * exposing standard Linux VFS file operations (open, read, write, unlocked_ioctl, release).
 *
 * Author: Tribhuwan Singh
 * Project: MediSave Edge Capstone Project
 * License: Dual MIT/GPL
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include <linux/string.h>

#include "../include/medisave_ioctl.h"

#define DRIVER_NAME "medisave"
#define CLASS_NAME  "medisave_class"
#define BUFFER_SIZE 256

/* Module Information */
MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Tribhuwan Singh <webosingh93@gmail.com>");
MODULE_DESCRIPTION("Linux Character Device Driver for MediSave Edge Medicine Storage Sensor");
MODULE_VERSION("1.0");

/* Device management state */
static dev_t dev_num;
static struct cdev medisave_cdev;
static struct class *medisave_class = NULL;
static struct device *medisave_device = NULL;

/* Concurrency protection */
static DEFINE_MUTEX(medisave_mutex);

/* Simulated hardware sensor state (represented in milli-Celsius: 6500 = 6.50 °C) */
/* Rule: No floating-point arithmetic inside Linux Kernel space */
static int current_temp_milli = 6500; // Default 6.50 °C (Standard cold chain: 2°C to 8°C)

/**
 * @brief Evaluates status string based on milli-Celsius temperature.
 * Thresholds:
 *   < 2000 mC (< 2.0 °C)     : LOW
 *   2000 - 8000 mC (2 - 8 °C): NORMAL
 *   8001 - 10000 mC (8 - 10 °C): WARNING
 *   > 10000 mC (> 10.0 °C)   : CRITICAL
 */
static const char* get_temperature_status(int temp_milli)
{
    if (temp_milli < 2000) {
        return "LOW";
    } else if (temp_milli <= 8000) {
        return "NORMAL";
    } else if (temp_milli <= 10000) {
        return "WARNING";
    } else {
        return "CRITICAL";
    }
}

/**
 * @brief Helper to parse fixed-point ASCII temperature string (e.g., "11.20", "6.5", "-3.5")
 * into signed milli-Celsius integer without floating-point math.
 *
 * Architecture Note — Why integer milli-Celsius representation is used:
 * Linux kernel code executes in Ring 0 where hardware floating-point registers (FPU/SIMD)
 * are not preserved across kernel preemption and context switches for performance reasons
 * and to prevent kernel FPU exceptions/traps. By scaling Celsius values by 1000 into fixed-point
 * integer milli-Celsius (e.g., 6.50 °C = 6500 mC), the driver achieves 3 decimal places of
 * physical precision using only standard ALU integer arithmetic.
 *
 * Prototype Threshold Note:
 * The prototype uses configurable temperature thresholds for demonstration.
 * Sample/default thresholds are used for the demo (e.g., 2.0 °C to 8.0 °C cold chain) and
 * should not be interpreted as universal storage requirements for all medicines.
 *
 * Strict Validation Rules:
 * - Reject NULL or empty input
 * - Allow optional leading sign (+ or -)
 * - Require at least one numeric digit
 * - Allow at most one decimal point with maximum 3 fractional digits
 * - Reject alphabetical characters and trailing garbage (e.g., "abc", "12xyz", ".", "+")
 * - Enforce prototype sensor range: -50.000 °C (-50000 mC) to 100.000 °C (100000 mC)
 *
 * Returns 0 on success, or negative error code (-EINVAL, -ERANGE) on failure.
 */
static int parse_temperature_to_milli(const char *str, int *out_milli)
{
    int sign = 1;
    long int_part = 0;
    int frac_part = 0;
    int frac_digits = 0;
    int digits_seen = 0;
    int i = 0;
    long total_milli = 0;

    if (!str || !out_milli) {
        return -EINVAL;
    }

    /* Trim leading whitespace */
    while (str[i] == ' ' || str[i] == '\t' || str[i] == '\r' || str[i] == '\n') {
        i++;
    }

    /* Check optional sign */
    if (str[i] == '-') {
        sign = -1;
        i++;
    } else if (str[i] == '+') {
        i++;
    }

    /* Parse integer component */
    while (str[i] >= '0' && str[i] <= '9') {
        int_part = int_part * 10 + (str[i] - '0');
        digits_seen++;
        i++;
        if (int_part > 100000) {
            return -ERANGE; /* Exceeds physical sensor capability */
        }
    }

    /* Parse optional fractional component */
    if (str[i] == '.') {
        i++;
        while (str[i] >= '0' && str[i] <= '9') {
            if (frac_digits < 3) {
                frac_part = frac_part * 10 + (str[i] - '0');
                frac_digits++;
            } else {
                /* More than 3 fractional digits: reject invalid precision */
                return -EINVAL;
            }
            digits_seen++;
            i++;
        }
    }

    /* Input must contain at least one valid digit */
    if (digits_seen == 0) {
        return -EINVAL;
    }

    /* Strip trailing whitespace */
    while (str[i] == ' ' || str[i] == '\t' || str[i] == '\r' || str[i] == '\n') {
        i++;
    }

    /* Any remaining character indicates trailing garbage (e.g. "12xyz") */
    if (str[i] != '\0') {
        return -EINVAL;
    }

    /* Normalize fraction to thousandths (milli-Celsius) */
    if (frac_digits == 1) {
        frac_part *= 100;
    } else if (frac_digits == 2) {
        frac_part *= 10;
    }

    total_milli = sign * (int_part * 1000 + frac_part);

    /* Enforce prototype storage range: -50.000 °C to 100.000 °C */
    if (total_milli < -50000 || total_milli > 100000) {
        return -ERANGE;
    }

    *out_milli = (int)total_milli;
    return 0;
}

/* ========================================================================= */
/* VFS File Operations Handlers                                              */
/* ========================================================================= */

static int medisave_open(struct inode *inodep, struct file *filep)
{
    pr_info("medisave: Device /dev/medisave opened by process %s (PID %d)\n",
            current->comm, current->pid);
    return 0;
}

static int medisave_release(struct inode *inodep, struct file *filep)
{
    pr_info("medisave: Device /dev/medisave closed by process %s (PID %d)\n",
            current->comm, current->pid);
    return 0;
}

static ssize_t medisave_read(struct file *filep, char __user *user_buf,
                             size_t count, loff_t *offset)
{
    char kbuf[BUFFER_SIZE];
    int len;
    int temp_milli;
    const char *status;
    int int_part, frac_part;
    int sign_flag = 0;

    mutex_lock(&medisave_mutex);
    temp_milli = current_temp_milli;
    status = get_temperature_status(temp_milli);
    mutex_unlock(&medisave_mutex);

    /* Format temperature representation */
    if (temp_milli < 0) {
        sign_flag = 1;
        temp_milli = -temp_milli;
    }
    int_part = temp_milli / 1000;
    frac_part = (temp_milli % 1000) / 10; // 2 decimal places

    len = snprintf(kbuf, BUFFER_SIZE,
                   "Temperature: %s%d.%02d C\nStatus: %s\n",
                   (sign_flag ? "-" : ""), int_part, frac_part, status);

    if (*offset >= len) {
        return 0; /* EOF */
    }

    if (count > (size_t)(len - *offset)) {
        count = len - *offset;
    }

    /* Transfer data to user space safely */
    if (copy_to_user(user_buf, kbuf + *offset, count)) {
        pr_err("medisave: Failed to copy sensor data to user space\n");
        return -EFAULT;
    }

    *offset += count;
    return count;
}

static ssize_t medisave_write(struct file *filep, const char __user *user_buf,
                              size_t count, loff_t *offset)
{
    char kbuf[BUFFER_SIZE];
    int parsed_milli = 0;
    const char *status;
    size_t copy_len;
    int parse_rc;

    if (count == 0) return -EINVAL;

    copy_len = (count < BUFFER_SIZE - 1) ? count : (BUFFER_SIZE - 1);

    if (copy_from_user(kbuf, user_buf, copy_len)) {
        pr_err("medisave: Failed to copy temperature from user space\n");
        return -EFAULT;
    }
    kbuf[copy_len] = '\0';

    parse_rc = parse_temperature_to_milli(kbuf, &parsed_milli);
    if (parse_rc == 0) {
        mutex_lock(&medisave_mutex);
        current_temp_milli = parsed_milli;
        status = get_temperature_status(current_temp_milli);
        mutex_unlock(&medisave_mutex);

        pr_info("medisave: Simulated temperature updated to %d.%02d C (Status: %s)\n",
                parsed_milli / 1000,
                abs((parsed_milli % 1000) / 10),
                status);
        return count;
    } else {
        pr_warn("medisave: Rejected malformed input string '%s' (error code: %d)\n", kbuf, parse_rc);
        return parse_rc;
    }
}

static long medisave_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    int temp_milli;
    char status_buf[MEDISAVE_STATUS_LEN];
    struct medisave_ioctl_data data;

    if (_IOC_TYPE(cmd) != MEDISAVE_IOC_MAGIC) {
        return -ENOTTY;
    }
    if (_IOC_NR(cmd) > MEDISAVE_IOC_MAXNR) {
        return -ENOTTY;
    }

    switch (cmd) {
    case MEDISAVE_IOC_SET_TEMP:
        if (copy_from_user(&temp_milli, (int __user *)arg, sizeof(int))) {
            return -EFAULT;
        }
        mutex_lock(&medisave_mutex);
        current_temp_milli = temp_milli;
        mutex_unlock(&medisave_mutex);
        pr_info("medisave: IOCTL set temperature -> %d.%02d C\n",
                temp_milli / 1000, abs((temp_milli % 1000) / 10));
        break;

    case MEDISAVE_IOC_GET_TEMP:
        mutex_lock(&medisave_mutex);
        temp_milli = current_temp_milli;
        mutex_unlock(&medisave_mutex);
        if (copy_to_user((int __user *)arg, &temp_milli, sizeof(int))) {
            return -EFAULT;
        }
        break;

    case MEDISAVE_IOC_GET_STATUS:
        mutex_lock(&medisave_mutex);
        strncpy(status_buf, get_temperature_status(current_temp_milli), sizeof(status_buf) - 1);
        status_buf[sizeof(status_buf) - 1] = '\0';
        mutex_unlock(&medisave_mutex);
        if (copy_to_user((char __user *)arg, status_buf, sizeof(status_buf))) {
            return -EFAULT;
        }
        break;

    case MEDISAVE_IOC_GET_DATA:
        mutex_lock(&medisave_mutex);
        data.temperature_milli = current_temp_milli;
        strncpy(data.status, get_temperature_status(current_temp_milli), sizeof(data.status) - 1);
        data.status[sizeof(data.status) - 1] = '\0';
        mutex_unlock(&medisave_mutex);
        if (copy_to_user((struct medisave_ioctl_data __user *)arg, &data, sizeof(data))) {
            return -EFAULT;
        }
        break;

    default:
        return -ENOTTY;
    }

    return 0;
}

/* File operations table */
static const struct file_operations medisave_fops = {
    .owner          = THIS_MODULE,
    .open           = medisave_open,
    .release        = medisave_release,
    .read           = medisave_read,
    .write          = medisave_write,
    .unlocked_ioctl = medisave_ioctl,
};

/* ========================================================================= */
/* Module Lifecycle: Initialization & Teardown                               */
/* ========================================================================= */

static int __init medisave_init(void)
{
    int ret;

    pr_info("medisave: Initializing MediSave Edge Character Device Driver...\n");

    /* 1. Dynamically allocate major/minor character device region */
    ret = alloc_chrdev_region(&dev_num, 0, 1, DRIVER_NAME);
    if (ret < 0) {
        pr_err("medisave: Failed to allocate character device region (%d)\n", ret);
        return ret;
    }
    pr_info("medisave: Allocated major number %d, minor %d\n",
            MAJOR(dev_num), MINOR(dev_num));

    /* 2. Initialize and bind cdev */
    cdev_init(&medisave_cdev, &medisave_fops);
    medisave_cdev.owner = THIS_MODULE;

    ret = cdev_add(&medisave_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("medisave: Failed to register cdev (%d)\n", ret);
        goto fail_cdev;
    }

    /* 3. Create device class (handling kernel API evolution) */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    medisave_class = class_create(CLASS_NAME);
#else
    medisave_class = class_create(THIS_MODULE, CLASS_NAME);
#endif
    if (IS_ERR(medisave_class)) {
        ret = PTR_ERR(medisave_class);
        pr_err("medisave: Failed to create device class (%d)\n", ret);
        goto fail_class;
    }

    /* 4. Create device node /dev/medisave */
    medisave_device = device_create(medisave_class, NULL, dev_num, NULL, DRIVER_NAME);
    if (IS_ERR(medisave_device)) {
        ret = PTR_ERR(medisave_device);
        pr_err("medisave: Failed to create device node /dev/%s (%d)\n", DRIVER_NAME, ret);
        goto fail_device;
    }

    pr_info("medisave: Device node /dev/%s successfully created.\n", DRIVER_NAME);
    pr_info("medisave: Driver loaded and ready for storage monitoring.\n");
    return 0;

fail_device:
    class_destroy(medisave_class);
fail_class:
    cdev_del(&medisave_cdev);
fail_cdev:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void __exit medisave_exit(void)
{
    pr_info("medisave: Unloading MediSave Edge Character Device Driver...\n");
    device_destroy(medisave_class, dev_num);
    class_destroy(medisave_class);
    cdev_del(&medisave_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("medisave: Device /dev/%s dismantled and unregistered. Goodbye.\n", DRIVER_NAME);
}

module_init(medisave_init);
module_exit(medisave_exit);

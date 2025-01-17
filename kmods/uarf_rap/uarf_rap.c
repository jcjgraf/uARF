/**
 * Run a user provided pointer in privilege mode.
 *
 * TODO: Fails when the kernel pagefaults while running in kernel mode
 */

#include "uarf_rap.h"
#include "linux/fs.h"
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/version.h>

static int major;
static struct class *cls;
static struct device *dev;
static unsigned long cr4;

static long rap_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    pr_info("IOCTL received");

    switch (cmd) {
    case IOCTL_RAP: {

        unsigned long cr4_mod = 0;
        struct rap_request *req;

        if (cr4 & (X86_CR4_SMEP | X86_CR4_SMAP)) {
            pr_debug("Need to temporarely disable SMEP&SMAP\n");
            // This wont work well on VMs since it results in TLB flush.
            cr4_mod = cr4 & ~(X86_CR4_SMEP | X86_CR4_SMAP);
        }

        preempt_disable();

        if (cr4_mod) {
            asm volatile("mov %0 , %%cr4" : "+r"(cr4_mod) : : "memory");
        }
        req = (struct rap_request *) arg;

        req->func(req->data);

        if (cr4_mod) {
            asm volatile("mov %0,%%cr4" : "+r"(cr4) : : "memory");
        }

        preempt_enable();
        pr_debug("Returned from function\n");

        break;
    }
    default: {
        pr_warn("Unsupported command encountered: %u\n", cmd);
        pr_debug("RAP: %lu\n", IOCTL_RAP);
        return -EINVAL;
    }
    }
    return 0;
}

static struct file_operations fops = {
    .unlocked_ioctl = rap_ioctl,
};

// Gives read/write access to all users
// Similar how it is none for tty driver
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0)
static char *devnode(const struct device *dev, umode_t *mode) {
#else
static char *devnode(struct device *dev, umode_t *mode) {
#endif
    if (mode) {
        *mode = 0666; // Set read/write permissions for all users
    }
    return NULL;
}

static int __init rap_init(void) {
    pr_debug("rap module init\n");

    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        pr_alert("Failed to register device %s\n", DEVICE_NAME);
        return major;
    }
    pr_debug("Registered device with major number %d\n", major);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    cls = class_create(DEVICE_NAME);
#else
    cls = class_create(THIS_MODULE, DEVICE_NAME);
#endif

    if (IS_ERR(cls)) {
        pr_alert("Failed to create class for device %s\n", DEVICE_NAME);
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(cls);
    }

    cls->devnode = devnode;

    dev = device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(dev)) {
        pr_alert("Failed to create device for device %s\n", DEVICE_NAME);
        class_destroy(cls);
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(dev);
    }

    pr_info("Device %s initialized successfully\n", DEVICE_NAME);

    cr4 = __read_cr4();
    if (cr4 & (X86_CR4_SMEP | X86_CR4_SMAP)) {
        pr_info("You will need to update cr4 (VMEXIT) to disable smep\n");
    }

    return 0;
}

static void __exit rap_exit(void) {
    pr_debug("rap module exit\n");
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, DEVICE_NAME);
    pr_info("Device %s exited successfully\n", DEVICE_NAME);
}

module_init(rap_init);
module_exit(rap_exit);

MODULE_LICENSE("GPL");

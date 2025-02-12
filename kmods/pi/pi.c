/***
 * Enable a user process to run privilege instructions.
 */

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include "pi.h"

static int major;
static struct class *cls;
static struct device *dev;

static inline unsigned long _uarf_read_cr3(void) {
    unsigned long cr3;

    asm volatile("mov %%cr3, %0" : "=r"(cr3));

    return cr3;
}

static inline void _uarf_write_cr3(unsigned long cr3) {
    asm volatile("mov %0, %%cr3" ::"r"(cr3));
}

static long uarf_pi_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    pr_info("PI IOCTL received\n");

    switch (cmd) {
    case UARF_IOCTL_RDMSR: {

        struct UarfPiReqMsr req;
        if (copy_from_user(&req, (struct msr_request __user *) arg, sizeof(req))) {
            pr_warn("Failed to copy data from user\n");
            return -EINVAL;
        }

        pr_debug("Type RDMSR: msr 0x%x\n", req.msr);
        rdmsrl(req.msr, req.value);
        pr_debug("Got value 0x%llx\n", req.value);

        if (copy_to_user((struct msr_request __user *) arg, &req, sizeof(req))) {
            pr_warn("Failed to copy data back to user\n");
            return -EINVAL;
        }

        break;
    }
    case UARF_IOCTL_WRMSR: {

        struct UarfPiReqMsr req;
        if (copy_from_user(&req, (struct msr_request __user *) arg, sizeof(req))) {
            pr_warn("Failed to copy data from user\n");
            return -EINVAL;
        }

        pr_debug("Type WRMSR: msr 0x%x, value 0x%llx\n", req.msr, req.value);
        wrmsrl(req.msr, req.value);

        break;
    }
    case UARF_IOCTL_INVLPG: {

        uint64_t addr;
        if (copy_from_user(&addr, (uint64_t __user *) arg, sizeof(uint64_t))) {
            pr_warn("Failed to copy data from user\n");
            return -EINVAL;
        }

        pr_debug("Type INVLPG: addr 0x%llx\n", addr);
        asm volatile("invlpg (%0)" ::"r"(addr) : "memory");

        break;
    }
    case UARF_IOCTL_FLUSH_TLB: {
        pr_debug("Type FLUSH_TLB\n");
        _uarf_write_cr3(_uarf_read_cr3());
        break;
    }
    case UARF_IOCTL_CPUID: {
        pr_debug("Type CPUID\n");
        struct UarfPiReqCpuid req;
        if (copy_from_user(&req, (uint64_t __user *) arg, sizeof(req))) {
            pr_warn("Failed to copy data from user\n");
            return -EINVAL;
        }

        cpuid(req.eax, &req.eax, &req.ebx, &req.ecx, &req.edx);

        if (copy_to_user((struct msr_request __user *) arg, &req, sizeof(req))) {
            pr_warn("Failed to copy data back to user\n");
            return -EINVAL;
        }
        break;
    }
    default: {
        pr_warn("Unsupported command encountered: %u\n", cmd);
        pr_debug("RDMSR: %lu\n", UARF_IOCTL_RDMSR);
        pr_debug("WRMSR: %lu\n", UARF_IOCTL_WRMSR);
        pr_debug("INVLPG: %lu\n", UARF_IOCTL_INVLPG);
        pr_debug("CPUID: %lu\n", UARF_IOCTL_CPUID);
        return -EINVAL;
    }
    }

    return 0;
}

static struct file_operations fops = {
    .unlocked_ioctl = uarf_pi_ioctl,
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

static int __init uarf_pi_init(void) {
    pr_debug("pi module initiated\n");

    major = register_chrdev(0, "pi", &fops);
    if (major < 0) {
        pr_alert("Failed to register device\n");
        return major;
    }
    pr_debug("Registered device with major number %d\n", major);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    cls = class_create("pi");
#else
    cls = class_create(THIS_MODULE, "pi");
#endif

    if (IS_ERR(cls)) {
        pr_alert("Failed to create class\n");
        unregister_chrdev(major, "pi");
        return PTR_ERR(cls);
    }

    cls->devnode = devnode;

    dev = device_create(cls, NULL, MKDEV(major, 0), NULL, "pi");
    if (IS_ERR(dev)) {
        pr_alert("Failed to create device\n");
        class_destroy(cls);
        unregister_chrdev(major, "pi");
        return PTR_ERR(dev);
    }

    pr_info("Device initialized successfully\n");
    return 0;
}

static void __exit uarf_pi_exit(void) {
    pr_debug("pi module exited\n");

    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, "pi");

    pr_info("Device exited successfully\n");
}

module_init(uarf_pi_init);
module_exit(uarf_pi_exit);

MODULE_LICENSE("GPL");

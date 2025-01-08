/*
 * Enable a user to run `rdmsr` and `wrmsr`.
 */

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include "uarf_msr.h"

static int major;
static struct class *cls;
static struct device *dev;

static long msr_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    pr_info("IOCTL received\n");

    struct msr_request req;

    if (copy_from_user(&req, (struct msr_request __user *) arg, sizeof(req))) {
        pr_warn("Failed to copy data from user\n");
        return -EINVAL;
    }

    switch (cmd) {
    case IOCTL_RDMSR: {
        pr_debug("Type RDMSR: msr 0x%x\n", req.msr);
        rdmsrl(req.msr, req.value);
        pr_debug("Got value 0x%llx\n", req.value);
        break;
    }
    case IOCTL_WRMSR: {
        pr_debug("Type WRMSR: msr 0x%x, value 0x%llx\n", req.msr, req.value);
        wrmsrl(req.msr, req.value);
        break;
    }
    default: {
        pr_warn("Unsupported command encountered: %d\n", cmd);
        return -EINVAL;
    }
    }

    if (copy_to_user((struct msr_request __user *) arg, &req, sizeof(req))) {
        pr_warn("Failed to copy data back to user\n");
        return -EINVAL;
    }

    return 0;
}

static struct file_operations fops = {
    .unlocked_ioctl = msr_ioctl,
};

// Gives read/write access to all users
// Similar how it is none for tty driver
static char *devnode(const struct device *dev, umode_t *mode) {
    if (mode) {
        *mode = 0666; // Set read/write permissions for all users
    }
    return NULL;
}

static int __init msr_init(void) {
    pr_debug("msr initiated\n");

    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        pr_alert("Failed to register device\n");
        return major;
    }
    pr_debug("Registered device with major number %d\n", major);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    cls = class_create(DEVICE_NAME);
#else
    cls = class_create(THIS_MODULE, DEVICE_NAME);
#endif

    if (IS_ERR(cls)) {
        pr_alert("Failed to create class\n");
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(cls);
    }

    cls->devnode = devnode;

    dev = device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(dev)) {
        pr_alert("Failed to create device\n");
        class_destroy(cls);
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(dev);
    }

    pr_info("Device initialized successfully\n");
    return 0;
}

static void __exit msr_exit(void) {
    pr_debug("msr exited\n");

    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, DEVICE_NAME);

    pr_info("Device exited successfully\n");
}

module_init(msr_init);
module_exit(msr_exit);

MODULE_LICENSE("GPL");

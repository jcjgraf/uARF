/**
 * Kernel module to run arbitrary code in SMM.
 *
 * This module assumes the presence of a special SMI handler.
 */

#include <asm/io.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/version.h>

#include "include/smm_ioctls.h"

#define SMM_HANDLER 0xb2

#define SMM_CMD_PING 0x11
#define SMM_CMD_REGISTER 0x12
#define SMM_CMD_RUN 0x13

#define DEV_NAME "uarf_smm"

static int major;
static struct class *cls;
static struct device *dev;

static uint64_t smm_buf_vaddr = 0;
static phys_addr_t smm_buf_paddr = 0;

static long smm_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    pr_debug("uarf_smm: ioctl received\n");
    (void)file;

    switch (cmd) {
    case UARF_SMM_IOCTL_PING: {
        pr_debug("uarf_smm ioctl type ping\n");

        uint64_t value = arg;
        if (copy_from_user(&value, (uint64_t *)arg, sizeof(value))) {
            pr_warn("uarf_smm: Failed to copy data from user\n");
            return -EINVAL;
        }

        pr_debug("uarf_smm: Send value 0x%llx\n", value);

        // clang-format off
        asm volatile (
            "out %[cmd], %[port]\n\t"
            : [val]"+c"(value)
            : [cmd]"a"(SMM_CMD_PING), [port]"Nd"(SMM_HANDLER)
            :
        );
        // clang-format on

        pr_debug("uarf_smm: Receive value 0x%llx\n", value);

        if (copy_to_user((uint64_t *)arg, &value, sizeof(value))) {
            pr_warn("uarf_smm: Failed to copy data back to user\n");
            return -EINVAL;
        }

        return 0;
    }
    case UARF_SMM_IOCTL_REGISTER: {
        pr_debug("uarf_smm: ioctl type REGISTER\n");

        ucode user_code;

        if (copy_from_user(&user_code, (const void *)arg, sizeof(user_code))) {
            pr_warn("uarf_smm: failed to copy from userspace");
            return -EINVAL;
        }

        // Allocate a page in the lower memory region
        if (!smm_buf_vaddr) {
            pr_debug("uarf_smm: Allocate SMM buffer\n");
            // Maps as NX
            smm_buf_vaddr = get_zeroed_page(GFP_DMA32);
            if (!smm_buf_vaddr) {
                pr_warn("uarf_smm: Failed to allocate SMM buffer\n");
                return -ENOMEM;
            }

            smm_buf_paddr = virt_to_phys((void *)smm_buf_vaddr);
        }

        pr_debug(
            "uarf_smm: Allocated SMM buffer at vaddr: 0x%llx, paddr: 0x%llx\n",
            smm_buf_vaddr, smm_buf_paddr);

        // Copy code from userspace to kernel buffer
        if (copy_from_user((void *)smm_buf_vaddr, (const void *)user_code.ptr,
                           user_code.size)) {
            pr_warn("uarf_smm: Failed to copy code from userspace\n");
            return -EFAULT;
        }

        pr_debug("uarf_smm: Copied %zu bytes from 0x%llx to 0x%llx (0x%llx)\n",
                 user_code.size, user_code.ptr, smm_buf_vaddr, smm_buf_paddr);

        // pr_debug("uarf_smm: Test code by calling it\n");
        // ((void (*)(void))smm_buf_vaddr)();
        // pr_debug("uarf_smm: Call succeeded\n");

        // clang-format off
        asm volatile (
            "out %0, %1\n\t"
            :: "a"(SMM_CMD_REGISTER), "Nd"(SMM_HANDLER), "c"(smm_buf_paddr)
            :
        );
        // clang-format on

        return 0;
    }
    case UARF_SMM_IOCTL_RUN: {
        pr_debug("uarf_smm: ioctl of type RUN\n");

        if (!smm_buf_vaddr) {
            pr_warn("uarf_smm: SMM buffer not initialized\n");
            return -EFAULT;
        }

        // clang-format off
        asm volatile (
            "out %0, %1\n\t"
            :: "a"(SMM_CMD_RUN), "Nd"(SMM_HANDLER)
            :
        );
        // clang-format on


        return 0;
    }
    default: {
        pr_warn("uarf_smm: illegal ioctl received: %u\n", cmd);
        pr_debug("RUN: %u\n", UARF_SMM_IOCTL_RUN);
        return -EINVAL;
    }
    }

    return 0;
}

// Gives read/write access to all users
// Similar how it is none for tty driver
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0)
static char *devnode(const struct device *dev, umode_t *mode) {
#else
static char *devnode(struct device *dev, umode_t *mode) {
#endif
    (void)dev;
    if (mode) {
        *mode = 0666; // Set read/write permissions for all users
    }
    return NULL;
}

static struct file_operations fops = {
    .unlocked_ioctl = smm_ioctl,
};

static int __init smm_init(void) {
    pr_debug("uarf_smm: module init\n");

    major = register_chrdev(0, DEV_NAME, &fops);
    if (major < 0) {
        pr_alert("uarf_smm: Failed to register device\n");
        return major;
    }
    pr_debug("uarf_smm: Registered device with major number %d\n", major);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    cls = class_create(DEV_NAME);
#else
    cls = class_create(THIS_MODULE, "smm");
#endif

    if (IS_ERR(cls)) {
        pr_alert("uarf_smm: Failed to create class\n");
        unregister_chrdev(major, DEV_NAME);
        return PTR_ERR(cls);
    }

    cls->devnode = devnode;

    dev = device_create(cls, NULL, MKDEV(major, 0), NULL, DEV_NAME);
    if (IS_ERR(dev)) {
        pr_alert("uarf_smm: Failed to create device\n");
        class_destroy(cls);
        unregister_chrdev(major, DEV_NAME);
        return PTR_ERR(dev);
    }

    pr_debug("uarf_smm: Device initialized successfully\n");

    return 0;
}

static void __exit smm_exit(void) {
    pr_debug("uarf_smm: module exit\n");

    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, DEV_NAME);

    pr_info("uarf_smm: Device exited successfully\n");
}

module_init(smm_init);
module_exit(smm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jean-Claude Graf");
MODULE_DESCRIPTION("uARF SMM helper module");

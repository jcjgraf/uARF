# Zen 4

## System Information
```bash
$ hostname
ee-tik-cn128
$ cat /proc/cmdline
BOOT_IMAGE=/boot/vmlinuz-6.8.0-51-generic root=UUID=86c98890-ca6c-4dd2-906b-d035b3230afb ro quiet nosmap nosmep amd-iommu clearcpuid=295,308 default_hugepagesz=1G hugepages=18
$ name -a
Linux ee-tik-cn128 6.8.0-51-generic #52-Ubuntu SMP PREEMPT_DYNAMIC Thu Dec  5 13:09:44 UTC 2024 x86_64 x86_64 x86_64 GNU/Linux
$ cat /etc/lsb-release
DISTRIB_ID=Ubuntu
DISTRIB_RELEASE=24.04
DISTRIB_CODENAME=noble
DISTRIB_DESCRIPTION="Ubuntu 24.04.2 LTS"
$ cat /proc/cpuinfo | grep 'microcode' | head -n 1
microcode       : 0xa601203
```

## Test Information
- `UARF_FRS_THRESHOLD 200`

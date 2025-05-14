# Zen 5

## System Information
```bash
$ hostname
ee-tik-cn145
$ cat /proc/cmdline
BOOT_IMAGE=/boot/vmlinuz-6.8.0-52-generic root=UUID=189f8691-3874-474e-a09a-85b0080d9096 ro quiet nosmap nosmep clearcpuid=295,308
$ name -a
Linux ee-tik-cn145 6.8.0-52-generic #53-Ubuntu SMP PREEMPT_DYNAMIC Sat Jan 11 00:06:25 UTC 2025 x86_64 x86_64 x86_64 GNU/Linux
$ cat /etc/lsb-release
DISTRIB_ID=Ubuntu
DISTRIB_RELEASE=24.04
DISTRIB_CODENAME=noble
DISTRIB_DESCRIPTION="Ubuntu 24.04.1 LTS"
$ cat /proc/cpuinfo | grep 'microcode' | head -n 1
microcode       : 0xb404023
```

## Test Information
- `UARF_FRS_THRESHOLD 200`

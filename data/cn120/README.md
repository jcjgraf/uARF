# Raptor Lake

## System Information
```sh
$ hostname
ee-tik-cn120
$ cat /proc/cmdline
BOOT_IMAGE=/boot/vmlinuz-6.8.0-47-generic root=UUID=1b9a160c-f511-459f-a940-05c7f5adc582 ro quiet nosmap nosmep clearcpuid=295,308
$ name -a
Linux ee-tik-cn120 6.8.0-47-generic #47~22.04.1-Ubuntu SMP PREEMPT_DYNAMIC Wed Oct  2 16:16:55 UTC 2 x86_64 x86_64 x86_64 GNU/Linux
$ cat /etc/lsb-release
DISTRIB_ID=Ubuntu
DISTRIB_RELEASE=22.04
DISTRIB_CODENAME=jammy
DISTRIB_DESCRIPTION="Ubuntu 22.04.5 LTS"
$ cat /proc/cpuinfo | grep 'microcode' | head -n 1
microcode       : 0x12c
```

### Test Information

#### Performance
- Signal in HU: `UARF_FRS_THRESHOLD 600`
- Signal in HS: `UARF_FRS_THRESHOLD 600`
- Signal in GU: `UARF_FRS_THRESHOLD `
- Signal in GS: `UARF_FRS_THRESHOLD `

#### Efficiency
- Signal in HU: `UARF_FRS_THRESHOLD 300`
- Signal in HS: `UARF_FRS_THRESHOLD 500`
- Signal in GU: `UARF_FRS_THRESHOLD 650`
- Signal in GS: `UARF_FRS_THRESHOLD 650`

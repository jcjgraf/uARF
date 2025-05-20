# Zen 2

## System Information
```bash
$ hostname
ee-tik-cn102
$ cat /proc/cmdline
BOOT_IMAGE=/boot/vmlinuz-6.5.0-35-generic root=UUID=6c2c3575-19fc-4fc4-8dc4-3978104cd1ea ro quiet splash vt.handoff=7
$ name -a
Linux ee-tik-cn102 6.5.0-35-generic #35~22.04.1-Ubuntu SMP PREEMPT_DYNAMIC Tue May  7 09:00:52 UTC 2 x86_64 x86_64 x86_64 GNU/Linux
$ cat /etc/lsb-release
DISTRIB_ID=Ubuntu
DISTRIB_RELEASE=22.04
DISTRIB_CODENAME=jammy
DISTRIB_DESCRIPTION="Ubuntu 22.04.2 LTS"
$ cat /proc/cpuinfo | grep 'microcode' | head -n 1
microcode       : 0x8301038
```

- Yes IBPB
- Yes IBRS
- Yes STIBP

## Test Information
- `UARF_FRS_THRESHOLD 600`

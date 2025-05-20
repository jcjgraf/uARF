# Coffee Lake

## System Information
```bash
$ hostname
ee-tik-cn016
$ cat /proc/cmdline
BOOT_IMAGE=/boot/vmlinuz-5.15.0-121-generic root=UUID=ccf13d5f-5588-41de-b6b6-16d002714a41 ro serial=tty0 console=ttyS0,115200n8 default_hugepagesz=1G hugepagesz=1G hugepages=1 transparent_hugepage=madvise mitigations=off noibrs noibpb nopti nospectre_v2 nospectre_v1 l1tf=off nospec_store_bypass_disable no_stf_barrier mds=off tsx=on tsx_async_abort=off quiet=0 quiet splash vt.handoff=7
$ name -a
Linux ee-tik-cn016 5.15.0-121-generic #131~20.04.1-Ubuntu SMP Mon Aug 12 13:09:56 UTC 2024 x86_64 x86_64 x86_64 GNU/Linux
$ cat /etc/lsb-release
DISTRIB_ID=Ubuntu
DISTRIB_RELEASE=20.04
DISTRIB_CODENAME=focal
DISTRIB_DESCRIPTION="Ubuntu 20.04.6 LTS"
$ cat /proc/cpuinfo | grep 'microcode' | head -n 1
microcode       : 0xd6
```

### Test Information

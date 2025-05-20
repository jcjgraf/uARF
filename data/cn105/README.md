# Zen 3

## System Information
```bash
$ hostname
ee-tik-cn105
$ cat /proc/cmdline
BOOT_IMAGE=/boot/vmlinuz-6.9.0-rc7-snp-host-05b10142ac6a root=UUID=3fd6a1e6-0102-4f03-926b-6ba07e0d3f4f ro text
$ name -a
Linux ee-tik-cn105 6.9.0-rc7-snp-host-05b10142ac6a #2 SMP Thu May 23 16:58:53 CEST 2024 x86_64 x86_64 x86_64 GNU/Linux
$ cat /etc/lsb-release
DISTRIB_ID=Ubuntu
DISTRIB_RELEASE=22.04
DISTRIB_CODENAME=jammy
DISTRIB_DESCRIPTION="Ubuntu 22.04.4 LTS"
$ cat /proc/cpuinfo | grep 'microcode' | head -n 1
microcode       : 0xa0011d3
```

## Test Information
- `UARF_FRS_THRESHOLD 400`

### fgetpriority

A small LKM to filter sys_getpriority / sys_setpriority / sys_ioprio_get / sys_ioprio_set

Usage (LKM):

```shell
insmod fgetpriority.ko

```

Usage (LKM, if above fails):
```shell
#!/bin/sh

kptr_set=$(cat /proc/sys/kernel/kptr_restrict)
echo 1 > /proc/sys/kernel/kptr_restrict
addr=$(grep " kallsyms_lookup_name$" /proc/kallsyms | awk {'print "0x"$1'})
echo $kptr_set > /proc/sys/kernel/kptr_restrict

insmod fgetpriority.ko kallsyms_lookup_name_fn="$addr"

```

Usage (built-in):
- just compile it in :^)

Compatibility:
- CONFIG_ARM64=y
- CONFIG_KALLSYMS=y
- CONFIG_KALLSYMS_ALL=y (if used as LKM)


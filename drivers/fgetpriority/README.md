### fgetpriority

A small LKM to filter sys_getpriority / sys_setpriority / sys_ioprio_get / sys_ioprio_set

Usage (if LKM):

```shell
#!/bin/sh

kptr_set=$(cat /proc/sys/kernel/kptr_restrict)
echo 1 > /proc/sys/kernel/kptr_restrict
native_sct=$(grep " sys_call_table$" /proc/kallsyms | awk {'print "0x"$1'})
compat_sct=$(grep " compat_sys_call_table$" /proc/kallsyms | awk {'print "0x"$1'})
echo $kptr_set > /proc/sys/kernel/kptr_restrict

insmod fgetpriority.ko native_sct_param="$native_sct" compat_sct_param="$compat_sct"

```
Note: if you do NOT want to hook compat_sys_call_table, just pass compat_sct_param=0x0

Compatibility:
- CONFIG_KALLSYMS_ALL=y is required if built as module.

Usage (if built-in):
- just compile it in :^)


